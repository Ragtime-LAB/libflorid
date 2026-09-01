#include "florid/detail/ArmImpl.hpp"
#include "florid/Exceptions.hpp"
#include "florid/detail/tick.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace florid {

namespace {

Version s_versionFromWire(const fci::Semver& s_version) noexcept {
    return Version{
        .m_major = s_version.major,
        .m_minor = s_version.minor,
        .m_patch = s_version.patch,
    };
}

FirmwareType s_firmwareTypeFromWire(std::uint8_t s_type) noexcept {
    switch (static_cast<fci::arm::FirmwareType>(s_type)) {
        case fci::arm::FirmwareType::StandardArm:
            return FirmwareType::kStandardArm;
        case fci::arm::FirmwareType::MobileArm:
            return FirmwareType::kMobileArm;
        case fci::arm::FirmwareType::CobotArm:
            return FirmwareType::kCobotArm;
    }
    return FirmwareType::kUnknown;
}

BusState s_busStateFromWire(std::uint8_t s_state) noexcept {
    switch (s_state) {
        case 0:
            return BusState::kErrorActive;
        case 1:
            return BusState::kErrorWarning;
        case 2:
            return BusState::kErrorPassive;
        case 3:
            return BusState::kBusOff;
        case 4:
            return BusState::kStopped;
        default:
            return BusState::kUnknown;
    }
}

template <std::size_t Size>
std::string s_stringFromWire(const std::array<char, Size>& s_bytes) {
    const auto s_end = std::find(s_bytes.begin(), s_bytes.end(), '\0');
    if (s_end == s_bytes.end() ||
        !std::all_of(s_end, s_bytes.end(),
                     [](char s_byte) { return s_byte == '\0'; })) {
        return {};
    }
    return std::string(s_bytes.begin(), s_end);
}

DeviceInfo s_deviceInfoFromWire(const fci::arm::DeviceInfo& s_info) {
    return DeviceInfo{
        .m_protocol_version = s_versionFromWire(s_info.protocol_version),
        .m_firmware_version = s_versionFromWire(s_info.fw_version),
        .m_board_name = s_stringFromWire(s_info.board_name),
        .m_custom_name = s_stringFromWire(s_info.custom_name),
        .m_firmware_type = s_firmwareTypeFromWire(s_info.fw_type),
    };
}

bool s_validSettings(const DeviceSettings& s_settings) noexcept {
    if (s_settings.m_firmware_period_us < 100 ||
        s_settings.m_firmware_period_us > 1'000'000) {
        return false;
    }
    for (const float s_scale : s_settings.m_gravity_scale) {
        if (!std::isfinite(s_scale)) return false;
    }
    for (const auto& s_fold : s_settings.m_torque_fold) {
        if (!std::isfinite(s_fold.m_continuous_torque) ||
            !std::isfinite(s_fold.m_peak_torque) ||
            !std::isfinite(s_fold.m_thermal_capacity) ||
            !std::isfinite(s_fold.m_torque_ramp_rate) ||
            s_fold.m_continuous_torque < 0 ||
            s_fold.m_peak_torque < s_fold.m_continuous_torque ||
            s_fold.m_thermal_capacity < 0 ||
            s_fold.m_torque_ramp_rate < 0) {
            return false;
        }
    }
    for (const auto& s_limits : s_settings.m_joint_limits) {
        if (!std::isfinite(s_limits.m_min) ||
            !std::isfinite(s_limits.m_max) ||
            s_limits.m_min > s_limits.m_max) {
            return false;
        }
    }
    return true;
}

bool s_deviceSettingsFromWire(const fci::arm::DeviceSettings& s_wire,
                              DeviceSettings& s_settings) noexcept {
    DeviceSettings s_converted{};
    s_converted.m_firmware_period_us = s_wire.firmware_dt_us;
    for (std::size_t s_index = 0;
         s_index < s_converted.m_gravity_scale.size(); ++s_index) {
        s_converted.m_gravity_scale[s_index] = s_wire.gravity_scale[s_index];
    }
    for (std::size_t s_index = 0;
         s_index < s_converted.m_torque_fold.size(); ++s_index) {
        const auto& s_source = s_wire.torque_fold[s_index];
        s_converted.m_torque_fold[s_index] = TorqueFoldParameters{
            .m_continuous_torque = s_source.t_cont,
            .m_peak_torque = s_source.t_peak,
            .m_thermal_capacity = s_source.thermal_capacity,
            .m_torque_ramp_rate = s_source.torque_ramp_rate,
        };
    }
    for (std::size_t s_index = 0;
         s_index < s_converted.m_joint_limits.size(); ++s_index) {
        s_converted.m_joint_limits[s_index] = JointLimits{
            .m_min = s_wire.joint_limits[s_index].first,
            .m_max = s_wire.joint_limits[s_index].second,
        };
    }
    if (!s_validSettings(s_converted)) return false;
    s_settings = s_converted;
    return true;
}

bool s_deviceSettingsToWire(const DeviceSettings& s_settings,
                            fci::arm::DeviceSettings& s_wire) noexcept {
    if (!s_validSettings(s_settings)) return false;
    s_wire = {};
    s_wire.firmware_dt_us = s_settings.m_firmware_period_us;
    for (std::size_t s_index = 0;
         s_index < s_settings.m_gravity_scale.size(); ++s_index) {
        s_wire.gravity_scale[s_index] = s_settings.m_gravity_scale[s_index];
    }
    for (std::size_t s_index = 0;
         s_index < s_settings.m_torque_fold.size(); ++s_index) {
        const auto& s_source = s_settings.m_torque_fold[s_index];
        s_wire.torque_fold[s_index] = fci::arm::TorqueFoldParams{
            .t_cont = s_source.m_continuous_torque,
            .t_peak = s_source.m_peak_torque,
            .thermal_capacity = s_source.m_thermal_capacity,
            .torque_ramp_rate = s_source.m_torque_ramp_rate,
        };
    }
    for (std::size_t s_index = 0;
         s_index < s_settings.m_joint_limits.size(); ++s_index) {
        const auto& s_source = s_settings.m_joint_limits[s_index];
        s_wire.joint_limits[s_index] = {s_source.m_min, s_source.m_max};
    }
    return true;
}

float s_finiteOrZero(float s_value) noexcept {
    return std::isfinite(s_value) ? s_value : 0.0f;
}

ArmDiagnostics s_diagnosticsFromWire(
    const fci::arm::ArmDiagnostics& s_diagnostics) noexcept {
    ArmDiagnostics s_converted{
        .m_uptime_s = s_diagnostics.uptime_s,
        .m_tick_count = s_diagnostics.tick_count,
        .m_mode_entry_ms = s_diagnostics.mode_entry_ms,
        .m_bus_healthy = s_diagnostics.bus_healthy == 1,
        .m_bus_state = s_busStateFromWire(s_diagnostics.bus_state),
        .m_tx_error_count = s_diagnostics.tx_err_count,
        .m_rx_error_count = s_diagnostics.rx_err_count,
    };
    for (std::size_t s_index = 0; s_index < s_converted.m_joints.size();
         ++s_index) {
        s_converted.m_joints[s_index] = JointDiagnostics{
            .m_healthy = s_diagnostics.joints[s_index].healthy == 1,
            .m_temperature_c =
                s_finiteOrZero(s_diagnostics.joints[s_index].temp_c),
        };
    }
    s_converted.m_gripper = GripperDiagnostics{
        .m_healthy = s_diagnostics.gripper.healthy == 1,
        .m_temperature_c = s_finiteOrZero(s_diagnostics.gripper.temp_c),
    };
    return s_converted;
}

std::optional<fci::arm::MotorRegister> s_motorRegisterToWire(
    MotorRegister s_register) noexcept {
    switch (s_register) {
        case MotorRegister::TorqueConstant:
            return fci::arm::MotorRegister::TorqueConstant;
        case MotorRegister::GearEfficiency:
            return fci::arm::MotorRegister::GearEfficiency;
        case MotorRegister::CurrentLoopBandwidth:
            return fci::arm::MotorRegister::CurrentLoopBandwidth;
        case MotorRegister::SpeedLoopKp:
            return fci::arm::MotorRegister::SpeedLoopKp;
        case MotorRegister::SpeedLoopKi:
            return fci::arm::MotorRegister::SpeedLoopKi;
        case MotorRegister::PositionLoopKp:
            return fci::arm::MotorRegister::PositionLoopKp;
        case MotorRegister::PositionLoopKi:
            return fci::arm::MotorRegister::PositionLoopKi;
        case MotorRegister::SpeedLoopDamping:
            return fci::arm::MotorRegister::SpeedLoopDamping;
        case MotorRegister::SpeedLoopFilterBW:
            return fci::arm::MotorRegister::SpeedLoopFilterBW;
        case MotorRegister::CurrentEnhanceFactor:
            return fci::arm::MotorRegister::CurrentEnhanceFactor;
        case MotorRegister::VelocityEnhanceFactor:
            return fci::arm::MotorRegister::VelocityEnhanceFactor;
        case MotorRegister::VoltageUnder:
            return fci::arm::MotorRegister::VoltageUnder;
        case MotorRegister::VoltageOver:
            return fci::arm::MotorRegister::VoltageOver;
        case MotorRegister::TemperatureLimit:
            return fci::arm::MotorRegister::TemperatureLimit;
        case MotorRegister::OvercurrentLimit:
            return fci::arm::MotorRegister::OvercurrentLimit;
        case MotorRegister::Acceleration:
            return fci::arm::MotorRegister::Acceleration;
        case MotorRegister::Deceleration:
            return fci::arm::MotorRegister::Deceleration;
        case MotorRegister::MaxSpeed:
            return fci::arm::MotorRegister::MaxSpeed;
        case MotorRegister::PositionMax:
            return fci::arm::MotorRegister::PositionMax;
        case MotorRegister::VelocityMax:
            return fci::arm::MotorRegister::VelocityMax;
        case MotorRegister::TorqueMax:
            return fci::arm::MotorRegister::TorqueMax;
        case MotorRegister::DampingCoefficient:
            return fci::arm::MotorRegister::DampingCoefficient;
        case MotorRegister::Inertia:
            return fci::arm::MotorRegister::Inertia;
        case MotorRegister::HardwareVersion:
            return fci::arm::MotorRegister::HardwareVersion;
        case MotorRegister::SoftwareVersion:
            return fci::arm::MotorRegister::SoftwareVersion;
        case MotorRegister::PolePairs:
            return fci::arm::MotorRegister::PolePairs;
        case MotorRegister::PhaseResistance:
            return fci::arm::MotorRegister::PhaseResistance;
        case MotorRegister::PhaseInductance:
            return fci::arm::MotorRegister::PhaseInductance;
        case MotorRegister::FluxLinkage:
            return fci::arm::MotorRegister::FluxLinkage;
        case MotorRegister::GearRatio:
            return fci::arm::MotorRegister::GearRatio;
        case MotorRegister::SubVersion:
            return fci::arm::MotorRegister::SubVersion;
        case MotorRegister::MotorPosition:
            return fci::arm::MotorRegister::MotorPosition;
        case MotorRegister::OutputPosition:
            return fci::arm::MotorRegister::OutputPosition;
    }
    return std::nullopt;
}

bool s_validJointId(std::uint8_t s_joint_id) noexcept {
    return s_joint_id >= 1 && s_joint_id <= 7;
}

} // namespace

// ────────────────────────────────────────────────────────
//  ArmControl
// ────────────────────────────────────────────────────────

Duration ArmControl::firmwarePeriod() const {
    return Duration::fromUSec(m_impl ? m_impl->m_fw_dt_us : 2000);
}

Duration ArmControl::stateAge() const {
    if (!m_impl) return Duration::fromMSec(0);
    return Duration::fromUSec(m_impl->m_latency.stateAgeUs(detail::s_nowUs()));
}

Duration ArmControl::estimatedLatency() const {
    if (!m_impl) return Duration::fromMSec(0);
    return Duration::fromUSec(
        static_cast<std::uint64_t>(m_impl->m_latency.estimatedLatencyMs() * 1000.0));
}

double ArmControl::receiveJitterUs() const {
    if (!m_impl) return 0.0;
    return m_impl->m_latency.receiveJitterUs();
}

double ArmControl::receiveHz() const {
    if (!m_impl) return 0.0;
    return m_impl->m_latency.receiveHz(detail::s_nowUs());
}

bool ArmControl::isReconnecting() const {
    return m_impl ? m_impl->m_reconnecting.load() : false;
}

void ArmControl::finishMotion() {
    if (m_impl) m_impl->m_stop_flag = true;
}

void ArmControl::stopControl() {
    if (m_impl) {
        m_impl->m_stop_flag = true;
        m_impl->m_running = false;
    }
}

// ────────────────────────────────────────────────────────
//  ArmImpl
// ────────────────────────────────────────────────────────

ArmImpl::ArmImpl(std::unique_ptr<Transport> s_transport)
    : m_transport(std::move(s_transport))
{
    m_arm_control.m_impl = this;

#ifdef FLORID_HAS_MPC
    m_mpc = std::make_unique<CartesianMPCSolver<WillowMPCTraits>>();
#endif

    m_session.on_send([this](const std::uint8_t* s_data, std::size_t s_size) {
        m_transport->send(s_data, s_size);
    });

    m_transport->setReceiveCallback(s_onPhysData, this);

    s_fetchDeviceInfo();
    s_fetchDeviceSettings();

    // ── Lifecycle: notify firmware SDK is connected ──
    {
        fci::arm::SdkClientConnectedRequestPacket s_req{};
        s_req.payload.dummy = 0;
        m_session.request(s_req, 50);
    }

    m_connected = true;
}

ArmImpl::~ArmImpl() {
    m_running = false;

    // ── Lifecycle: notify firmware SDK disconnected (best-effort) ──
    {
        fci::arm::SdkClientDisconnectedRequestPacket s_req{};
        s_req.payload.dummy = 0;
        m_session.request(s_req, 20);
    }
}

void ArmImpl::s_onPhysData(void* s_context, const std::uint8_t* s_data, std::size_t s_size) {
    auto* s_self = static_cast<ArmImpl*>(s_context);
    s_self->s_feedBytes(s_data, s_size);
}

void ArmImpl::s_feedBytes(const std::uint8_t* s_data, std::size_t s_size) {
    auto s_result = m_session.receive(s_data, s_size);
    if (!s_result) return;

    auto s_status = m_session.deserializer().get<fci::arm::ArmStatus>();
    if (s_status.seq != m_last_status_seq) {
        m_last_status_seq = s_status.seq;
        m_latency.markReceived(s_status.last_sdk_timestamp_us, detail::s_nowUs());
        if (!m_rx_queue.enqueue(s_status)) return;
        m_data_ready.release();
    }

    auto s_diag = m_session.deserializer().get<fci::arm::ArmDiagnostics>();
    if (s_diag.tick_count != m_last_diag_tick) {
        m_last_diag_tick = s_diag.tick_count;
        m_last_diag = s_diagnosticsFromWire(s_diag);
    }
}

void ArmImpl::s_fetchDeviceInfo() {
    fci::arm::GetDeviceInfoRequestPacket s_req{};
    s_req.payload.dummy = 0;

    // Firmware sends GetDeviceInfoResponse directly without USBAck,
    // so use notify() + manual req_id + poll (same as readMotorRegister).
    s_req.req_id = m_session.ack_manager().allocate();
    (void)m_session.notify(s_req);

    using namespace std::chrono;
    auto s_deadline = steady_clock::now() + milliseconds(2000);

    while (steady_clock::now() < s_deadline) {
        auto s_response =
            m_session.deserializer().get<fci::arm::GetDeviceInfoResponsePacket>();
        if (s_response.req_id == s_req.req_id) {
            m_device_info = s_deviceInfoFromWire(s_response.payload.info);
            return;
        }
        std::this_thread::yield();
    }

    // If device doesn't support GetDeviceInfo, use default values
    // and continue with the connection.
    m_device_info = DeviceInfo{
        .m_protocol_version = s_versionFromWire(fci::arm::kProtocolVersion),
        .m_firmware_type = FirmwareType::kStandardArm,
    };
}

void ArmImpl::s_fetchDeviceSettings() {
    fci::arm::GetDeviceSettingsRequestPacket s_req{};
    s_req.payload.dummy = 0;

    s_req.req_id = m_session.ack_manager().allocate();
    (void)m_session.notify(s_req);

    using namespace std::chrono;
    auto s_deadline = steady_clock::now() + milliseconds(2000);

    while (steady_clock::now() < s_deadline) {
        auto s_response =
            m_session.deserializer().get<fci::arm::GetDeviceSettingsResponsePacket>();
        if (s_response.req_id == s_req.req_id) {
            DeviceSettings s_settings{};
            if (s_deviceSettingsFromWire(s_response.payload.settings,
                                         s_settings)) {
                m_device_settings = s_settings;
                m_fw_dt_us = m_device_settings.m_firmware_period_us;
            } else {
                m_device_settings = DeviceSettings{};
                m_fw_dt_us = m_device_settings.m_firmware_period_us;
            }
            return;
        }
        std::this_thread::yield();
    }

    // If device doesn't support GetDeviceSettings, use default values
    m_device_settings = DeviceSettings{};
    m_fw_dt_us = m_device_settings.m_firmware_period_us;
}

bool ArmImpl::setDeviceSettings(const DeviceSettings& s_settings) {
    fci::arm::SetDeviceSettingsRequestPacket s_req{};
    if (!s_deviceSettingsToWire(s_settings, s_req.payload.settings)) {
        return false;
    }

    auto s_result = m_session.request(s_req, 200);
    if (!s_result) return false;

    if (*s_result == static_cast<std::uint8_t>(fci::arm::SetDeviceSettingsStatus::Ok)) {
        m_device_settings = s_settings;
        m_fw_dt_us = m_device_settings.m_firmware_period_us;
        return true;
    }
    return false;
}

ArmState ArmImpl::s_convertStatus(const fci::arm::ArmStatus& s_raw) {
    ArmState s_state;
    s_state.m_time = detail::get_tick_ms();
    s_state.m_seq = s_raw.seq;
    s_state.m_mode = static_cast<std::uint32_t>(static_cast<std::uint8_t>(s_raw.mode));
    s_state.m_source_timestamp_us = s_raw.timestamp_us;
    s_state.m_errors = s_raw.errors;

    for (int s_i = 0; s_i < 6; ++s_i) {
        s_state.m_q[s_i]  = s_raw.status.q[s_i];
        s_state.m_dq[s_i] = s_raw.status.dq[s_i];
        s_state.m_tau[s_i] = s_raw.status.tau[s_i];
    }
    for (int s_i = 0; s_i < 3; ++s_i)
        s_state.m_base_gravity[s_i] = s_raw.base_gravity[s_i];
    for (int s_i = 0; s_i < 16; ++s_i)
        s_state.m_O_T_EE[s_i] = s_raw.O_T_EE[s_i];
    for (int s_i = 0; s_i < 6; ++s_i)
        s_state.m_F_ext[s_i] = s_raw.F_ext[s_i];

    s_state.m_gripper_q   = s_raw.gripper.q;
    s_state.m_gripper_dq  = s_raw.gripper.dq;
    s_state.m_gripper_tau = s_raw.gripper.tau;

    return s_state;
}

ArmState ArmImpl::readOnce() {
    fci::arm::ArmStatus s_raw;
    if (!m_rx_queue.try_dequeue(s_raw)) return ArmState{};
    return s_convertStatus(s_raw);
}

ArmDiagnostics ArmImpl::readDiagnostics() {
    return m_last_diag;
}

// ────────────────────────────────────────────────────────
//  Control mode switch
// ────────────────────────────────────────────────────────

void ArmImpl::s_ensureMode(fci::arm::MotorControlMode s_mode) {
    if (s_mode == m_current_mode) return;

    fci::arm::ArmControlModeRequestPacket s_req{};
    s_req.payload.mode = s_mode;
    m_session.notify(s_req);  // fire-and-forget, no blocking

    m_current_mode = s_mode;
}

// ────────────────────────────────────────────────────────
//  Cartesian → Joint conversion (MPC or fallback)
// ────────────────────────────────────────────────────────

JointPVT ArmImpl::s_convertCartesian(const CartesianPose& s_cmd,
                                      const ArmState& s_state) {
#ifdef FLORID_HAS_MPC
    if (m_mpc) {
        return m_mpc->solve(s_state.m_q, s_state.m_dq, s_cmd.m_T);
    }
#endif
    JointPVT s_out{};
    return s_out;
}

void ArmImpl::s_sendCommand(const CartesianPose& s_cmd) {
#ifdef FLORID_HAS_MPC
    ArmState s_state = s_convertStatus(
        m_session.deserializer().get<fci::arm::ArmStatus>());
    auto s_joint = s_convertCartesian(s_cmd, s_state);
    s_sendCommand(s_joint);
#else
    if (s_supportsCartesian()) {
        auto s_pkt = m_arm_core.s_pack(s_cmd);
        s_pkt.sdk_timestamp_us = detail::s_nowUs();
        m_session.notify(s_pkt);
    } else {
        auto s_joint = s_convertCartesian(s_cmd, s_convertStatus(
            m_session.deserializer().get<fci::arm::ArmStatus>()));
        s_sendCommand(s_joint);
    }
#endif
}

void ArmImpl::s_sendCommand(const CartesianVelocities& s_cmd) {
    auto s_pkt = m_arm_core.s_pack(s_cmd);
    s_pkt.sdk_timestamp_us = detail::s_nowUs();
    m_session.notify(s_pkt);
}

// ────────────────────────────────────────────────────────
//  Configuration commands
// ────────────────────────────────────────────────────────

void ArmImpl::home() {
    fci::arm::HomeAllRequestPacket s_req{};
    s_req.payload.dummy = 0;
    auto s_result = m_session.request(s_req, 10000);

    if (!s_result) {
        throw CommandException("HomeAll request failed: ack timeout or transport error");
    }
}

void ArmImpl::enable() {
    fci::arm::SetArmModeRequestPacket s_req{};
    s_req.payload.mode = fci::arm::ArmMode::Pc;
    auto s_r = m_session.request(s_req, 500);
    if (!s_r) throw CommandException("enable failed: ack timeout");
}

void ArmImpl::drag() {
    fci::arm::SetArmModeRequestPacket s_req{};
    s_req.payload.mode = fci::arm::ArmMode::Drag;
    auto s_r = m_session.request(s_req, 500);
    if (!s_r) throw CommandException("drag failed: ack timeout");
}

void ArmImpl::disable() {
    fci::arm::SetArmModeRequestPacket s_req{};
    s_req.payload.mode = fci::arm::ArmMode::Damp;
    auto s_r = m_session.request(s_req, 500);
    if (!s_r) throw CommandException("disable failed: ack timeout");
}

void ArmImpl::s_requestPcMode() {
    fci::arm::SetArmModeRequestPacket s_req{};
    s_req.payload.mode = fci::arm::ArmMode::Pc;
    m_session.request(s_req, 200);
}

void ArmImpl::setJointImpedance(const float (&)[6]) {
    // Stub — SetJointImpedance not yet implemented in firmware protocol
}

void ArmImpl::setCartesianImpedance(const float (&)[6]) {
    // Stub
}

void ArmImpl::setEEFrame(const float (&)[16]) {
    // Stub
}

void ArmImpl::setLoad(float, const float (&)[3], const float (&)[9]) {
    // Stub
}

void ArmImpl::automaticErrorRecovery() {
    // Send ClearFaults, then ClearError
    fci::arm::ClearFaultsRequestPacket s_req_fault{};
    m_session.request(s_req_fault, 100);

    for (int s_joint = 0; s_joint < 6; ++s_joint) {
        fci::arm::ClearErrorRequestPacket s_req{};
        s_req.payload.joint_id = static_cast<std::uint8_t>(s_joint);
        m_session.request(s_req, 100);
    }
}

void ArmImpl::stop() {
    m_stop_flag = true;
    m_running = false;
    m_data_ready.release();
}

// ────────────────────────────────────────────────────────
//  Motor register access
// ────────────────────────────────────────────────────────

std::optional<float> ArmImpl::readMotorRegister(std::uint8_t s_joint_id,
                                                 MotorRegister s_rid) {
    const auto s_wire_register = s_motorRegisterToWire(s_rid);
    if (!s_validJointId(s_joint_id) || !s_wire_register) return std::nullopt;

    fci::arm::MotorRegisterReadRequestPacket s_req{};
    s_req.payload.joint_id = s_joint_id;
    s_req.payload.rid = static_cast<std::uint8_t>(*s_wire_register);

    // notify() + poll: firmware sends MotorRegisterReadResponse (0x621E)
    // without a separate USBAck, so request() would time out on wait_ack.
    s_req.req_id = m_session.ack_manager().allocate();
    (void)m_session.notify(s_req);

    using namespace std::chrono;
    auto s_deadline = steady_clock::now() + milliseconds(200);

    while (steady_clock::now() < s_deadline) {
        auto s_resp =
            m_session.deserializer().get<fci::arm::MotorRegisterReadResponsePacket>();
        if (s_resp.req_id == s_req.req_id) {
            if (s_resp.payload.status == fci::arm::MotorRegisterStatus::Ok &&
                s_resp.payload.joint_id == s_joint_id &&
                s_resp.payload.rid == s_req.payload.rid &&
                std::isfinite(s_resp.payload.value)) {
                return s_resp.payload.value;
            }
            return std::nullopt;
        }
        std::this_thread::yield();
    }

    return std::nullopt;
}

bool ArmImpl::writeMotorRegister(std::uint8_t s_joint_id,
                                  MotorRegister s_rid,
                                  float s_value) {
    const auto s_wire_register = s_motorRegisterToWire(s_rid);
    if (!s_validJointId(s_joint_id) || !s_wire_register ||
        !std::isfinite(s_value)) {
        return false;
    }

    fci::arm::MotorRegisterWriteRequestPacket s_req{};
    s_req.payload.joint_id = s_joint_id;
    s_req.payload.rid = static_cast<std::uint8_t>(*s_wire_register);
    s_req.payload.value = s_value;

    auto s_result = m_session.request(s_req, 200);
    if (!s_result) return false;

    return *s_result == static_cast<std::uint8_t>(fci::arm::AckStatus::Ok);
}

bool ArmImpl::storeParameters(std::uint8_t s_joint_id) {
    if (!s_validJointId(s_joint_id)) return false;
    fci::arm::MotorStoreParamsRequestPacket s_req{};
    s_req.payload.joint_id = s_joint_id;

    auto s_result = m_session.request(s_req, 200);
    if (!s_result) return false;

    return *s_result == static_cast<std::uint8_t>(fci::arm::AckStatus::Ok);
}

bool ArmImpl::setZeroPoint(std::uint8_t s_joint_id) {
    if (!s_validJointId(s_joint_id)) return false;
    fci::arm::MotorSetZeroRequestPacket s_req{};
    s_req.payload.joint_id = s_joint_id;

    auto s_result = m_session.request(s_req, 200);
    if (!s_result) return false;

    return *s_result == static_cast<std::uint8_t>(fci::arm::AckStatus::Ok);
}

} // namespace florid
