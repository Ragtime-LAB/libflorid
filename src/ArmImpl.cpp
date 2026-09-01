#include "florid/detail/ArmImpl.hpp"

#include "florid/Exceptions.hpp"

#include <chrono>
#include <cmath>
#include <string>

namespace florid {

namespace {

using namespace std::chrono_literals;

constexpr std::uint32_t s_kLeaseDurationMs = 2000;
constexpr std::uint32_t s_kHomeLeaseDurationMs = 15'000;
constexpr std::uint32_t s_kDefaultRpcTimeoutMs = 500;

std::string s_operationError(const char* s_operation,
                             detail::FciEndpointStatus s_status,
                             const detail::FciOperationResult* s_result =
                                 nullptr) {
    std::string s_message{s_operation};
    s_message += " failed (endpoint=";
    s_message += std::to_string(static_cast<unsigned>(s_status));
    if (s_result != nullptr) {
        s_message += ", domain=";
        s_message += std::to_string(s_result->m_domain_status);
        s_message += ", link=";
        s_message += std::to_string(s_result->m_link_status);
    }
    s_message += ')';
    return s_message;
}

} // namespace

Duration ArmControl::firmwarePeriod() const {
    return Duration::fromUSec(m_impl ? m_impl->m_fw_dt_us : 2000);
}

Duration ArmControl::stateAge() const {
    if (!m_impl) return Duration::fromMSec(0);
    std::lock_guard<std::mutex> s_lock(m_impl->m_latency_mutex);
    return Duration::fromUSec(
        m_impl->m_latency.stateAgeUs(detail::s_nowUs()));
}

Duration ArmControl::estimatedLatency() const {
    if (!m_impl) return Duration::fromMSec(0);
    std::lock_guard<std::mutex> s_lock(m_impl->m_latency_mutex);
    return Duration::fromUSec(static_cast<std::uint64_t>(
        m_impl->m_latency.estimatedLatencyMs() * 1000.0));
}

double ArmControl::receiveJitterUs() const {
    if (!m_impl) return 0.0;
    std::lock_guard<std::mutex> s_lock(m_impl->m_latency_mutex);
    return m_impl->m_latency.receiveJitterUs();
}

double ArmControl::receiveHz() const {
    if (!m_impl) return 0.0;
    std::lock_guard<std::mutex> s_lock(m_impl->m_latency_mutex);
    return m_impl->m_latency.receiveHz(detail::s_nowUs());
}

bool ArmControl::isReconnecting() const {
    return m_impl ? m_impl->m_reconnecting.load() : false;
}

void ArmControl::finishMotion() {
    if (m_impl) m_impl->m_stop_flag = true;
}

void ArmControl::stopControl() {
    if (!m_impl) return;
    m_impl->m_stop_flag = true;
    m_impl->m_running = false;
    m_impl->m_data_ready.release();
}

ArmImpl::ArmImpl(std::unique_ptr<Transport> s_transport)
    : m_transport(std::move(s_transport)) {
    if (!m_transport) {
        throw NetworkException("ArmImpl requires a transport");
    }

    m_arm_control.m_impl = this;
#ifdef FLORID_HAS_MPC
    m_mpc = std::make_unique<CartesianMPCSolver<WillowMPCTraits>>();
#endif

    const auto s_initialized = m_endpoint.initialize();
    if (s_initialized != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(
            s_operationError("Wirelink endpoint initialization",
                             s_initialized));
    }
    const auto s_sink = m_endpoint.setSink(s_wireSink, this);
    if (s_sink != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(
            s_operationError("Wirelink transport sink setup", s_sink));
    }
    const auto s_callbacks =
        m_endpoint.setCallbacks(s_onArmStatus, s_onDiagnostics, this);
    if (s_callbacks != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(
            s_operationError("Wirelink callback setup", s_callbacks));
    }

    m_transport->setReceiveCallback(s_onPhysData, this);
    const auto s_started = m_endpoint.start();
    if (s_started != detail::FciEndpointStatus::kOk) {
        m_transport->setReceiveCallback(nullptr, nullptr);
        throw ProtocolException(
            s_operationError("Wirelink endpoint start", s_started));
    }

    try {
        // A connection is established only after the peer has granted the
        // control lease and the typed metadata RPCs have completed.
        s_requireOperation(
            m_endpoint.acquireControlLease(s_kLeaseDurationMs, 1000), 1500ms,
            "AcquireControlLease");
        s_fetchDeviceInfo();
        s_fetchDeviceSettings();
        m_connected.store(true, std::memory_order_release);
    } catch (...) {
        m_transport->setReceiveCallback(nullptr, nullptr);
        m_endpoint.stop();
        throw;
    }
}

ArmImpl::~ArmImpl() {
    m_connected.store(false, std::memory_order_release);
    stop();

    const auto s_lease = m_endpoint.controlLease();
    if (s_lease.m_token != 0) {
        const auto s_submit = m_endpoint.releaseControlLease(100);
        (void)s_operationSucceeded(s_submit, 250ms);
    }

    if (m_transport) {
        m_transport->setReceiveCallback(nullptr, nullptr);
    }
    m_endpoint.stop();
}

void ArmImpl::s_onPhysData(void* s_context, const std::uint8_t* s_data,
                           std::size_t s_size) noexcept {
    if (s_context == nullptr) return;
    static_cast<ArmImpl*>(s_context)->s_feedBytes(s_data, s_size);
}

wl_sink_result_t ArmImpl::s_wireSink(void* s_context, wl_io_token_t,
                                     const std::uint8_t* s_data,
                                     std::size_t s_size) noexcept {
    if (s_context == nullptr || s_data == nullptr || s_size == 0) {
        return WL_SINK_FAILED;
    }
    try {
        auto& s_self = *static_cast<ArmImpl*>(s_context);
        return s_self.m_transport->send(s_data, s_size) ? WL_SINK_SENT
                                                        : WL_SINK_FAILED;
    } catch (...) {
        return WL_SINK_FAILED;
    }
}

void ArmImpl::s_onArmStatus(
    void* s_context,
    const detail::FciArmStatusSnapshot& s_status) noexcept {
    if (s_context == nullptr) return;
    auto& s_self = *static_cast<ArmImpl*>(s_context);
    {
        std::lock_guard<std::mutex> s_lock(s_self.m_snapshot_mutex);
        s_self.m_latest_state = s_status.m_state;
        s_self.m_latest_state_generation = s_status.m_generation;
    }
    {
        std::lock_guard<std::mutex> s_lock(s_self.m_latency_mutex);
        s_self.m_latency.markReceived(s_status.m_last_sdk_timestamp_us,
                                      detail::s_nowUs());
    }
    if (!s_self.m_state_wake_pending.exchange(
            true, std::memory_order_acq_rel)) {
        s_self.m_data_ready.release();
    }
}

void ArmImpl::s_onDiagnostics(
    void* s_context, const ArmDiagnostics& s_diagnostics) noexcept {
    if (s_context == nullptr) return;
    auto& s_self = *static_cast<ArmImpl*>(s_context);
    std::lock_guard<std::mutex> s_lock(s_self.m_snapshot_mutex);
    s_self.m_last_diagnostics = s_diagnostics;
}

void ArmImpl::s_feedBytes(const std::uint8_t* s_data,
                          std::size_t s_size) noexcept {
    std::size_t s_offset = 0;
    while (s_offset < s_size) {
        std::size_t s_accepted = 0;
        const auto s_status = m_endpoint.feedBytes(
            s_data + s_offset, s_size - s_offset, s_accepted);
        s_offset += s_accepted;
        if (s_status != detail::FciEndpointStatus::kOk || s_accepted == 0) {
            break;
        }
    }
}

void ArmImpl::s_fetchDeviceInfo() {
    const auto s_submit = m_endpoint.getDeviceInfo(1000);
    if (s_submit.m_status != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(
            s_operationError("GetDeviceInfo submit", s_submit.m_status));
    }

    detail::FciOperationResult s_result{};
    const auto s_wait =
        m_endpoint.waitOperation(s_submit.m_request_id, 1500ms, s_result);
    DeviceInfo s_info{};
    const auto s_take = m_endpoint.takeDeviceInfo(
        s_submit.m_request_id, s_result, s_info);
    if (s_wait != detail::FciEndpointStatus::kOk ||
        s_take != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(
            s_operationError("GetDeviceInfo", s_take, &s_result));
    }
    m_device_info = std::move(s_info);
}

void ArmImpl::s_fetchDeviceSettings() {
    const auto s_submit = m_endpoint.getDeviceSettings(1000);
    if (s_submit.m_status != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(s_operationError("GetDeviceSettings submit",
                                                 s_submit.m_status));
    }

    detail::FciOperationResult s_result{};
    const auto s_wait =
        m_endpoint.waitOperation(s_submit.m_request_id, 1500ms, s_result);
    DeviceSettings s_settings{};
    const auto s_take = m_endpoint.takeDeviceSettings(
        s_submit.m_request_id, s_result, s_settings);
    if (s_wait != detail::FciEndpointStatus::kOk ||
        s_take != detail::FciEndpointStatus::kOk) {
        throw ProtocolException(
            s_operationError("GetDeviceSettings", s_take, &s_result));
    }
    m_device_settings = s_settings;
    m_fw_dt_us = s_settings.m_firmware_period_us;
}

bool ArmImpl::setDeviceSettings(const DeviceSettings& s_settings) {
    if (!s_operationSucceeded(
            m_endpoint.setDeviceSettings(s_settings, s_kDefaultRpcTimeoutMs),
            750ms)) {
        return false;
    }
    m_device_settings = s_settings;
    m_fw_dt_us = s_settings.m_firmware_period_us;
    return true;
}

ArmState ArmImpl::readOnce() {
    ArmState s_state{};
    (void)s_takeLatestState(s_state);
    return s_state;
}

ArmDiagnostics ArmImpl::readDiagnostics() {
    std::lock_guard<std::mutex> s_lock(m_snapshot_mutex);
    return m_last_diagnostics;
}

void ArmImpl::s_requireOperation(detail::FciSubmitResult s_submit,
                                 std::chrono::milliseconds s_wait,
                                 const char* s_operation) {
    if (s_submit.m_status != detail::FciEndpointStatus::kOk) {
        throw CommandException(
            s_operationError(s_operation, s_submit.m_status));
    }

    detail::FciOperationResult s_result{};
    const auto s_wait_status =
        m_endpoint.waitOperation(s_submit.m_request_id, s_wait, s_result);
    if (s_wait_status == detail::FciEndpointStatus::kBusy) {
        throw CommandException(
            s_operationError(s_operation, s_wait_status, &s_result));
    }
    const auto s_take =
        m_endpoint.takeOperation(s_submit.m_request_id, s_result);
    if (s_take != detail::FciEndpointStatus::kOk) {
        throw CommandException(
            s_operationError(s_operation, s_take, &s_result));
    }
}

bool ArmImpl::s_operationSucceeded(detail::FciSubmitResult s_submit,
                                   std::chrono::milliseconds s_wait) noexcept {
    if (s_submit.m_status != detail::FciEndpointStatus::kOk) return false;
    detail::FciOperationResult s_result{};
    const auto s_wait_status =
        m_endpoint.waitOperation(s_submit.m_request_id, s_wait, s_result);
    if (s_wait_status == detail::FciEndpointStatus::kBusy) return false;
    return m_endpoint.takeOperation(s_submit.m_request_id, s_result) ==
           detail::FciEndpointStatus::kOk;
}

void ArmImpl::s_requireCommand(detail::FciEndpointStatus s_status,
                               const char* s_operation) {
    if (s_status != detail::FciEndpointStatus::kOk) {
        throw CommandException(s_operationError(s_operation, s_status));
    }
    std::lock_guard<std::mutex> s_lock(m_latency_mutex);
    m_latency.markSent(detail::s_nowUs());
}

void ArmImpl::s_requestPcMode() {
    s_requireOperation(m_endpoint.setArmMode(detail::FciArmMode::kPc,
                                             s_kDefaultRpcTimeoutMs),
                       750ms, "SetArmMode(Pc)");
}

void ArmImpl::s_ensureMode(detail::FciMotorControlMode s_mode) {
    std::lock_guard<std::mutex> s_lock(m_control_mutex);
    if (m_current_mode == s_mode) return;
    s_requireOperation(
        m_endpoint.setArmControlMode(s_mode, s_kDefaultRpcTimeoutMs), 750ms,
        "SetArmControlMode");
    m_current_mode = s_mode;
}

void ArmImpl::s_ensureGripperMode(detail::FciMotorControlMode s_mode) {
    std::lock_guard<std::mutex> s_lock(m_control_mutex);
    if (m_current_gripper_mode == s_mode) return;
    s_requireOperation(
        m_endpoint.setGripperControlMode(s_mode, s_kDefaultRpcTimeoutMs),
        750ms, "SetGripperControlMode");
    m_current_gripper_mode = s_mode;
}

ArmState ArmImpl::s_latestState() const {
    std::lock_guard<std::mutex> s_lock(m_snapshot_mutex);
    return m_latest_state;
}

bool ArmImpl::s_takeLatestState(ArmState& s_state) noexcept {
    std::lock_guard<std::mutex> s_lock(m_snapshot_mutex);
    if (m_latest_state_generation == 0 ||
        m_latest_state_generation == m_consumed_state_generation) {
        return false;
    }
    s_state = m_latest_state;
    m_consumed_state_generation = m_latest_state_generation;
    return true;
}

JointPVT ArmImpl::s_convertCartesian(const CartesianPose& s_command,
                                     const ArmState& s_state) {
#ifdef FLORID_HAS_MPC
    if (m_mpc) {
        return m_mpc->solve(s_state.m_q, s_state.m_dq, s_command.m_T);
    }
#else
    (void)s_command;
    (void)s_state;
#endif
    return JointPVT{};
}

void ArmImpl::s_sendCommand(const JointMIT& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendJointMit(s_command, m_fw_dt_us, s_now),
                     "JointMitCommand");
}

void ArmImpl::s_sendCommand(const JointPosVel& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendJointPositionVelocity(s_command, s_now),
                     "JointPositionVelocityCommand");
}

void ArmImpl::s_sendCommand(const JointVel& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendJointVelocity(s_command, s_now),
                     "JointVelocityCommand");
}

void ArmImpl::s_sendCommand(const JointPVT& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendJointPvt(s_command, s_now),
                     "JointPvtCommand");
}

void ArmImpl::s_sendCommand(const CartesianPose& s_command) {
#ifdef FLORID_HAS_MPC
    s_sendCommand(s_convertCartesian(s_command, s_latestState()));
#else
    if (s_supportsCartesian()) {
        const auto s_now = detail::s_nowUs();
        s_requireCommand(
            m_endpoint.sendCartesianPose(s_command, m_fw_dt_us, s_now),
            "CartesianPoseCommand");
    } else {
        s_sendCommand(s_convertCartesian(s_command, s_latestState()));
    }
#endif
}

void ArmImpl::s_sendCommand(const CartesianVelocities& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(
        m_endpoint.sendCartesianVelocity(s_command, m_fw_dt_us, s_now),
        "CartesianVelocityCommand");
}

void ArmImpl::s_sendGripperCommand(const JointMIT& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendGripperMit(s_command, m_fw_dt_us, s_now),
                     "GripperMitCommand");
}

void ArmImpl::s_sendGripperCommand(const JointPosVel& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(
        m_endpoint.sendGripperPositionVelocity(s_command, s_now),
        "GripperPositionVelocityCommand");
}

void ArmImpl::s_sendGripperCommand(const JointVel& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendGripperVelocity(s_command, s_now),
                     "GripperVelocityCommand");
}

void ArmImpl::s_sendGripperCommand(const JointPVT& s_command) {
    const auto s_now = detail::s_nowUs();
    s_requireCommand(m_endpoint.sendGripperPvt(s_command, s_now),
                     "GripperPvtCommand");
}

void ArmImpl::home() {
    // The link currently serializes reliable RPCs. Extend the short control
    // lease before Home so its allowed ten-second response window cannot block
    // the renewal RPC long enough to expire the lease.
    s_requireOperation(
        m_endpoint.acquireControlLease(s_kHomeLeaseDurationMs, 1000), 1500ms,
        "ExtendControlLeaseForHome");
    try {
        s_requireOperation(m_endpoint.home(10'000), 10'500ms, "Home");
    } catch (...) {
        (void)s_operationSucceeded(
            m_endpoint.acquireControlLease(s_kLeaseDurationMs, 1000), 1500ms);
        throw;
    }
    s_requireOperation(
        m_endpoint.acquireControlLease(s_kLeaseDurationMs, 1000), 1500ms,
        "RestoreControlLeaseAfterHome");
}

void ArmImpl::enable() {
    s_requireOperation(
        m_endpoint.setArmMode(detail::FciArmMode::kPc,
                              s_kDefaultRpcTimeoutMs),
        750ms, "Enable");
}

void ArmImpl::drag() {
    s_requireOperation(
        m_endpoint.setArmMode(detail::FciArmMode::kDrag,
                              s_kDefaultRpcTimeoutMs),
        750ms, "Drag");
}

void ArmImpl::disable() {
    s_requireOperation(
        m_endpoint.setArmMode(detail::FciArmMode::kDamp,
                              s_kDefaultRpcTimeoutMs),
        750ms, "Disable");
}

void ArmImpl::automaticErrorRecovery() {
    s_requireOperation(m_endpoint.clearFaults(s_kDefaultRpcTimeoutMs), 750ms,
                       "ClearFaults");
    for (std::uint8_t s_joint = 0; s_joint < 6; ++s_joint) {
        s_requireOperation(
            m_endpoint.clearError(s_joint, s_kDefaultRpcTimeoutMs), 750ms,
            "ClearError");
    }
}

void ArmImpl::stop() {
    m_stop_flag.store(true, std::memory_order_release);
    m_running.store(false, std::memory_order_release);
    m_data_ready.release();
}

bool ArmImpl::s_validJointId(std::uint8_t s_joint_id) noexcept {
    return s_joint_id >= 1 && s_joint_id <= 7;
}

std::uint8_t ArmImpl::s_wireJointId(std::uint8_t s_joint_id) noexcept {
    return static_cast<std::uint8_t>(s_joint_id - 1);
}

std::optional<float> ArmImpl::readMotorRegister(std::uint8_t s_joint_id,
                                                MotorRegister s_register) {
    if (!s_validJointId(s_joint_id)) return std::nullopt;
    const auto s_submit = m_endpoint.readMotorRegister(
        s_wireJointId(s_joint_id), static_cast<std::uint8_t>(s_register),
        s_kDefaultRpcTimeoutMs);
    if (s_submit.m_status != detail::FciEndpointStatus::kOk) {
        return std::nullopt;
    }

    detail::FciOperationResult s_result{};
    const auto s_wait =
        m_endpoint.waitOperation(s_submit.m_request_id, 750ms, s_result);
    float s_value{};
    const auto s_take = m_endpoint.takeMotorRegister(
        s_submit.m_request_id, s_result, s_value);
    if (s_wait != detail::FciEndpointStatus::kOk ||
        s_take != detail::FciEndpointStatus::kOk ||
        !std::isfinite(s_value)) {
        return std::nullopt;
    }
    return s_value;
}

bool ArmImpl::writeMotorRegister(std::uint8_t s_joint_id,
                                 MotorRegister s_register, float s_value) {
    return s_validJointId(s_joint_id) && std::isfinite(s_value) &&
           s_operationSucceeded(
               m_endpoint.writeMotorRegister(
                   s_wireJointId(s_joint_id),
                   static_cast<std::uint8_t>(s_register), s_value,
                   s_kDefaultRpcTimeoutMs),
               750ms);
}

bool ArmImpl::storeParameters(std::uint8_t s_joint_id) {
    return s_validJointId(s_joint_id) &&
           s_operationSucceeded(
               m_endpoint.storeMotorParameters(s_wireJointId(s_joint_id),
                                               s_kDefaultRpcTimeoutMs),
               750ms);
}

bool ArmImpl::setZeroPoint(std::uint8_t s_joint_id) {
    return s_validJointId(s_joint_id) &&
           s_operationSucceeded(
               m_endpoint.setMotorZero(s_wireJointId(s_joint_id),
                                       s_kDefaultRpcTimeoutMs),
               750ms);
}

} // namespace florid
