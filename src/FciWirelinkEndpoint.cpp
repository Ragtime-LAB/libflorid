#include "florid/detail/FciWirelinkEndpoint.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

namespace florid::detail {

namespace {

constexpr std::size_t s_kNoSlot = FciWirelinkEndpoint::s_kOperationCapacity;
constexpr std::uint32_t s_kMaximumRelativeTimeout = UINT32_C(0x7fffffff);

std::uint32_t s_queueRemaining(wl_time_ms_t s_now, wl_time_ms_t s_submitted,
                               std::uint32_t s_timeout) noexcept {
    // A producer may submit after this owner pass sampled its clock. Treat
    // that small negative age as zero, not as an unsigned counter wrap.
    const auto s_age = static_cast<std::int32_t>(s_now - s_submitted);
    return s_age <= 0 ? s_timeout : s_timeout - std::min(
        s_timeout, static_cast<std::uint32_t>(s_age));
}

template <typename Range>
bool s_allFinite(const Range& s_values) noexcept {
    for (const float s_value : s_values) {
        if (!std::isfinite(s_value)) return false;
    }
    return true;
}

std::uint64_t s_steadyNowMs() noexcept {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count());
}

bool s_validArmStatus(const arm_status_t& s_status) noexcept {
    return s_status.has_mode && s_status.mode >= ARM_MODE_PC &&
           s_status.mode <= ARM_MODE_TELEOP && s_status.has_sequence &&
           s_status.has_timestamp_us && s_status.has_joint_position &&
           s_status.has_joint_velocity && s_status.has_joint_torque &&
           s_status.has_base_gravity && s_status.has_gripper_position &&
           s_status.has_gripper_velocity && s_status.has_gripper_torque &&
           s_status.has_end_effector_transform &&
           s_status.has_external_wrench && s_status.has_error_flags &&
           s_status.has_last_sdk_timestamp_us &&
           s_allFinite(s_status.joint_position) &&
           s_allFinite(s_status.joint_velocity) &&
           s_allFinite(s_status.joint_torque) &&
           s_allFinite(s_status.base_gravity) &&
           std::isfinite(s_status.gripper_position) &&
           std::isfinite(s_status.gripper_velocity) &&
           std::isfinite(s_status.gripper_torque) &&
           s_allFinite(s_status.end_effector_transform) &&
           s_allFinite(s_status.external_wrench);
}

FciArmStatusSnapshot s_armStatusFromWire(const arm_status_t& s_status,
                                         std::uint64_t s_generation) noexcept {
    FciArmStatusSnapshot s_snapshot{};
    s_snapshot.m_state.m_time = s_steadyNowMs();
    s_snapshot.m_state.m_seq = s_status.sequence;
    s_snapshot.m_state.m_mode = static_cast<std::uint32_t>(s_status.mode);
    s_snapshot.m_state.m_source_timestamp_us = s_status.timestamp_us;
    s_snapshot.m_state.m_errors = s_status.error_flags;
    std::copy_n(s_status.joint_position, 6, s_snapshot.m_state.m_q);
    std::copy_n(s_status.joint_velocity, 6, s_snapshot.m_state.m_dq);
    std::copy_n(s_status.joint_torque, 6, s_snapshot.m_state.m_tau);
    std::copy_n(s_status.base_gravity, 3,
                s_snapshot.m_state.m_base_gravity);
    std::copy_n(s_status.end_effector_transform, 16,
                s_snapshot.m_state.m_O_T_EE);
    std::copy_n(s_status.external_wrench, 6,
                s_snapshot.m_state.m_F_ext);
    s_snapshot.m_state.m_gripper_q = s_status.gripper_position;
    s_snapshot.m_state.m_gripper_dq = s_status.gripper_velocity;
    s_snapshot.m_state.m_gripper_tau = s_status.gripper_torque;
    s_snapshot.m_last_sdk_timestamp_us = s_status.last_sdk_timestamp_us;
    s_snapshot.m_generation = s_generation;
    return s_snapshot;
}

BusState s_busState(std::uint8_t s_value) noexcept {
    switch (s_value) {
        case 0: return BusState::kErrorActive;
        case 1: return BusState::kErrorWarning;
        case 2: return BusState::kErrorPassive;
        case 3: return BusState::kBusOff;
        case 4: return BusState::kStopped;
        default: return BusState::kUnknown;
    }
}

bool s_diagnosticsFromWire(const arm_diagnostics_t& s_wire,
                           ArmDiagnostics& s_domain) noexcept {
    if (!s_wire.has_uptime_s || !s_wire.has_tick_count ||
        !s_wire.has_mode_entry_ms || !s_wire.has_bus_healthy ||
        !s_wire.has_bus_state || !s_wire.has_tx_error_count ||
        !s_wire.has_rx_error_count || !s_wire.has_joint_healthy_mask ||
        !s_wire.has_joint_temperature_c || !s_wire.has_gripper_healthy ||
        !s_wire.has_gripper_temperature_c || !s_wire.has_overheat_mask ||
        !s_allFinite(s_wire.joint_temperature_c) ||
        !std::isfinite(s_wire.gripper_temperature_c)) {
        return false;
    }

    s_domain = ArmDiagnostics{
        .m_uptime_s = s_wire.uptime_s,
        .m_tick_count = s_wire.tick_count,
        .m_mode_entry_ms = s_wire.mode_entry_ms,
        .m_bus_healthy = s_wire.bus_healthy,
        .m_bus_state =
            s_busState(static_cast<std::uint8_t>(s_wire.bus_state)),
        .m_tx_error_count =
            static_cast<std::uint16_t>(s_wire.tx_error_count),
        .m_rx_error_count =
            static_cast<std::uint16_t>(s_wire.rx_error_count),
    };
    for (std::size_t s_index = 0; s_index < s_domain.m_joints.size();
         ++s_index) {
        s_domain.m_joints[s_index] = JointDiagnostics{
            .m_healthy =
                (s_wire.joint_healthy_mask & (UINT32_C(1) << s_index)) != 0,
            .m_temperature_c = s_wire.joint_temperature_c[s_index],
            .m_overheated =
                (s_wire.overheat_mask & (UINT32_C(1) << s_index)) != 0,
        };
    }
    s_domain.m_gripper = GripperDiagnostics{
        .m_healthy = s_wire.gripper_healthy,
        .m_temperature_c = s_wire.gripper_temperature_c,
        .m_overheated = (s_wire.overheat_mask & UINT32_C(0x40)) != 0,
    };
    return true;
}

FirmwareType s_firmwareType(firmware_type_t s_type) noexcept {
    switch (s_type) {
        case FIRMWARE_STANDARD_ARM: return FirmwareType::kStandardArm;
        case FIRMWARE_MOBILE_ARM: return FirmwareType::kMobileArm;
        case FIRMWARE_COBOT_ARM: return FirmwareType::kCobotArm;
        default: return FirmwareType::kUnknown;
    }
}

arm_mode_t s_armMode(FciArmMode s_mode) noexcept {
    switch (s_mode) {
        case FciArmMode::kPc: return ARM_MODE_PC;
        case FciArmMode::kDrag: return ARM_MODE_DRAG;
        case FciArmMode::kDamp: return ARM_MODE_DAMP;
        case FciArmMode::kRetracting: return ARM_MODE_RETRACTING;
        case FciArmMode::kTeleop: return ARM_MODE_TELEOP;
    }
    return ARM_MODE_PC;
}

motor_control_mode_t s_controlMode(FciMotorControlMode s_mode) noexcept {
    switch (s_mode) {
        case FciMotorControlMode::kMit: return MOTOR_CONTROL_MIT;
        case FciMotorControlMode::kPositionVelocity:
            return MOTOR_CONTROL_POSITION_VELOCITY;
        case FciMotorControlMode::kVelocity: return MOTOR_CONTROL_VELOCITY;
        case FciMotorControlMode::kPvt: return MOTOR_CONTROL_PVT;
    }
    return MOTOR_CONTROL_MIT;
}

bool s_validSettings(const DeviceSettings& s_settings) noexcept {
    if (s_settings.m_firmware_period_us < 100 ||
        s_settings.m_firmware_period_us > 1'000'000 ||
        !s_allFinite(s_settings.m_gravity_scale)) {
        return false;
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
            s_limits.m_min >= s_limits.m_max) {
            return false;
        }
    }
    return true;
}

template <typename Settings>
bool s_settingsFromWire(const Settings& s_wire,
                        DeviceSettings& s_settings) noexcept {
    if (!s_wire.has_firmware_dt_us || !s_wire.has_gravity_scale ||
        !s_wire.has_torque_continuous || !s_wire.has_torque_peak ||
        !s_wire.has_thermal_capacity || !s_wire.has_torque_ramp_rate ||
        !s_wire.has_joint_limit_min || !s_wire.has_joint_limit_max) {
        return false;
    }
    DeviceSettings s_domain{};
    s_domain.m_firmware_period_us = s_wire.firmware_dt_us;
    std::copy_n(s_wire.gravity_scale, 6, s_domain.m_gravity_scale.begin());
    for (std::size_t s_index = 0; s_index < 7; ++s_index) {
        s_domain.m_torque_fold[s_index] = TorqueFoldParameters{
            .m_continuous_torque = s_wire.torque_continuous[s_index],
            .m_peak_torque = s_wire.torque_peak[s_index],
            .m_thermal_capacity = s_wire.thermal_capacity[s_index],
            .m_torque_ramp_rate = s_wire.torque_ramp_rate[s_index],
        };
    }
    for (std::size_t s_index = 0; s_index < 6; ++s_index) {
        s_domain.m_joint_limits[s_index] = JointLimits{
            .m_min = s_wire.joint_limit_min[s_index],
            .m_max = s_wire.joint_limit_max[s_index],
        };
    }
    if (!s_validSettings(s_domain)) return false;
    s_settings = s_domain;
    return true;
}

template <typename Settings>
void s_settingsToWire(const DeviceSettings& s_settings,
                      Settings& s_wire) noexcept {
    s_wire = {};
    s_wire.has_firmware_dt_us = true;
    s_wire.firmware_dt_us = s_settings.m_firmware_period_us;
    s_wire.has_gravity_scale = true;
    std::copy(s_settings.m_gravity_scale.begin(),
              s_settings.m_gravity_scale.end(), s_wire.gravity_scale);
    s_wire.has_torque_continuous = true;
    s_wire.has_torque_peak = true;
    s_wire.has_thermal_capacity = true;
    s_wire.has_torque_ramp_rate = true;
    for (std::size_t s_index = 0; s_index < 7; ++s_index) {
        const auto& s_fold = s_settings.m_torque_fold[s_index];
        s_wire.torque_continuous[s_index] = s_fold.m_continuous_torque;
        s_wire.torque_peak[s_index] = s_fold.m_peak_torque;
        s_wire.thermal_capacity[s_index] = s_fold.m_thermal_capacity;
        s_wire.torque_ramp_rate[s_index] = s_fold.m_torque_ramp_rate;
    }
    s_wire.has_joint_limit_min = true;
    s_wire.has_joint_limit_max = true;
    for (std::size_t s_index = 0; s_index < 6; ++s_index) {
        s_wire.joint_limit_min[s_index] =
            s_settings.m_joint_limits[s_index].m_min;
        s_wire.joint_limit_max[s_index] =
            s_settings.m_joint_limits[s_index].m_max;
    }
}

template <typename String>
bool s_copyString(const String& s_source, char* s_destination,
                  std::size_t s_capacity,
                  std::uint8_t& s_size) noexcept {
    if (s_source.length >= s_capacity) {
        return false;
    }
    if (s_source.length != 0) {
        std::memcpy(s_destination, s_source.data, s_source.length);
    }
    s_destination[s_source.length] = '\0';
    s_size = static_cast<std::uint8_t>(s_source.length);
    return true;
}

} // namespace

FciWirelinkEndpoint::~FciWirelinkEndpoint() {
    stop();
}

FciEndpointStatus FciWirelinkEndpoint::initialize(
    const FciWirelinkEndpointConfig& s_config) {
    if (s_config.m_ack_timeout_ms == 0 ||
        s_config.m_ack_timeout_ms >= s_kMaximumRelativeTimeout) {
        return FciEndpointStatus::kInvalidArgument;
    }
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (m_initialized) return FciEndpointStatus::kBusy;
    }

    auto s_environment = wl_platform_environment();
    s_environment.session = s_config.m_session_source;
    s_environment.clock = {[](void*) -> wl_time_ms_t { return s_nowMs(); }, nullptr};
    fci_arm_endpoint_config_t s_endpoint_config;
    int s_result = fci_arm_endpoint_config_defaults(&s_endpoint_config, s_environment);
    if (s_result != WL_OK) return s_endpointStatus(s_result);
    s_endpoint_config.link.integrity = WL_INTEGRITY_NONE;
    s_endpoint_config.link.max_retries = s_config.m_max_retries;
    s_endpoint_config.link.ack_timeout_ms = s_config.m_ack_timeout_ms;
    s_endpoint_config.on_result = s_onResult;
    s_endpoint_config.user_data = this;
    s_result = fci_arm_endpoint_init_config(&m_endpoint, &s_endpoint_config);
    if (s_result != WL_OK) return s_endpointStatus(s_result);
    m_endpoint_storage_bytes = sizeof(m_endpoint);

    const wl_endpoint_policy_t s_policy{
        .user_data = this,
        .on_event = s_onEvent,
        .progress = s_applicationProgress,
        .deadline_hint = s_applicationDeadline,
    };
    s_result = wl_endpoint_set_policy(fci_arm_endpoint_handle(&m_endpoint), &s_policy);
    if (s_result != WL_OK) return s_endpointStatus(s_result);
    wl_pump_hooks_t s_adapter{};
    s_adapter.adapter_user_data = this;
    s_adapter.service = s_transportService;
    s_adapter.quiesce = s_quiesce;
    s_adapter.adapter_deadline_hint = s_transportDeadline;
    s_result = wl_endpoint_attach(fci_arm_endpoint_handle(&m_endpoint), &s_adapter);
    if (s_result != WL_OK) return s_endpointStatus(s_result);
    s_result = m_executor.initialize(fci_arm_endpoint_driver(&m_endpoint));
    if (s_result != WL_OK) return s_endpointStatus(s_result);

    m_command_capabilities.store(0U, std::memory_order_relaxed);

    std::lock_guard<std::mutex> s_lock(m_mutex);
    m_initialized = true;
    return FciEndpointStatus::kOk;
}

FciEndpointStatus FciWirelinkEndpoint::setSink(wl_sink_fn s_sink,
                                               void* s_user_data) noexcept {
    if (!m_initialized || s_sink == nullptr) {
        return FciEndpointStatus::kInvalidArgument;
    }
    return s_endpointStatus(m_executor.setSink(s_sink, s_user_data));
}

FciEndpointStatus FciWirelinkEndpoint::attachDirectTransport(
    Transport& s_transport) noexcept {
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (!m_initialized || m_running || m_direct_transport != nullptr ||
            !s_transport.usesDirectWirelink()) {
            return FciEndpointStatus::kNotReady;
        }
    }

    const int s_result = s_transport.attachWirelink(
        m_executor.context(), s_transportWake, this);
    if (s_result != WL_OK) return s_endpointStatus(s_result);

    std::lock_guard<std::mutex> s_lock(m_mutex);
    m_direct_transport = &s_transport;
    return FciEndpointStatus::kOk;
}

FciEndpointStatus FciWirelinkEndpoint::setCallbacks(
    ArmStatusCallback s_arm_status, DiagnosticsCallback s_diagnostics,
    void* s_user_data) noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    if (!m_initialized || m_running) return FciEndpointStatus::kNotReady;
    m_arm_status_callback = s_arm_status;
    m_diagnostics_callback = s_diagnostics;
    m_callback_user_data = s_user_data;
    return FciEndpointStatus::kOk;
}

FciEndpointStatus FciWirelinkEndpoint::start() noexcept {
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (!m_initialized || m_running) return FciEndpointStatus::kNotReady;
        // Publish the running state before creating the owner thread so
        // setup-only callback pointers cannot change underneath it.
        m_running = true;
    }
    const int s_result = m_executor.start();
    if (s_result != WL_OK) {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        m_running = false;
        return s_endpointStatus(s_result);
    }
    return FciEndpointStatus::kOk;
}

void FciWirelinkEndpoint::stop() noexcept {
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (!m_initialized) return;
    }
    m_executor.stop();
    std::lock_guard<std::mutex> s_lock(m_mutex);
    m_running = false;
}

FciEndpointStatus FciWirelinkEndpoint::feedBytes(
    const std::uint8_t* s_data, std::size_t s_size,
    std::size_t& s_accepted) noexcept {
    return s_endpointStatus(m_executor.feedBytes(s_data, s_size, s_accepted));
}

FciSubmitResult FciWirelinkEndpoint::acquireControlLease(
    std::uint32_t s_requested_timeout_ms,
    std::uint32_t s_rpc_timeout_ms) noexcept {
    if (s_requested_timeout_ms == 0 ||
        s_requested_timeout_ms >= s_kMaximumRelativeTimeout ||
        s_rpc_timeout_ms == 0 || s_rpc_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }

    std::uint64_t s_current_token{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        switch (m_lease.m_state) {
            case FciControlLeaseState::kAcquireQueued:
            case FciControlLeaseState::kAcquiring:
            case FciControlLeaseState::kRenewQueued:
            case FciControlLeaseState::kRenewing:
            case FciControlLeaseState::kReleaseQueued:
            case FciControlLeaseState::kReleasing:
                return {FciEndpointStatus::kBusy, 0};
            default:
                break;
        }
        if (m_lease.m_state == FciControlLeaseState::kHeld) {
            s_current_token = m_lease.m_token;
        }
    }
    OperationRequest s_request{};
    s_request.m_requested_lease_timeout_ms = s_requested_timeout_ms;
    s_request.m_lease_token = s_current_token;
    return s_submit(RpcKind::kAcquireLease, s_rpc_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::releaseControlLease(
    std::uint32_t s_rpc_timeout_ms) noexcept {
    if (s_rpc_timeout_ms == 0 ||
        s_rpc_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    std::uint64_t s_token{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if ((m_lease.m_state != FciControlLeaseState::kHeld &&
             m_lease.m_state != FciControlLeaseState::kRenewQueued &&
             m_lease.m_state != FciControlLeaseState::kRenewing) ||
            m_lease.m_token == 0) {
            return {FciEndpointStatus::kNoLease, 0};
        }
        s_token = m_lease.m_token;
    }
    OperationRequest s_request{};
    s_request.m_lease_token = s_token;
    return s_submit(RpcKind::kReleaseLease, s_rpc_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::getDeviceInfo(
    std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    return s_submit(RpcKind::kGetDeviceInfo, s_timeout_ms,
                    OperationRequest{}, false);
}

FciSubmitResult FciWirelinkEndpoint::setDeviceInfo(
    std::string_view s_custom_name, std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout ||
        s_custom_name.size() > s_kMaximumDeviceNameBytes) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    if (!s_custom_name.empty()) {
        std::memcpy(s_request.m_custom_name.data(), s_custom_name.data(),
                    s_custom_name.size());
    }
    s_request.m_custom_name_size =
        static_cast<std::uint8_t>(s_custom_name.size());
    return s_submit(RpcKind::kSetDeviceInfo, s_timeout_ms, s_request, false);
}

FciSubmitResult FciWirelinkEndpoint::getDeviceSettings(
    std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    return s_submit(RpcKind::kGetDeviceSettings, s_timeout_ms,
                    OperationRequest{}, false);
}

FciSubmitResult FciWirelinkEndpoint::setDeviceSettings(
    const DeviceSettings& s_settings, std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout ||
        !s_validSettings(s_settings)) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_device_settings = s_settings;
    return s_submit(RpcKind::kSetDeviceSettings, s_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::setArmControlMode(
    FciMotorControlMode s_mode, std::uint32_t s_timeout_ms) noexcept {
    const auto s_value = static_cast<std::uint8_t>(s_mode);
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout ||
        s_value < 1 || s_value > 4) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_control_mode = s_mode;
    return s_submit(RpcKind::kSetArmControlMode, s_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::setGripperControlMode(
    FciMotorControlMode s_mode, std::uint32_t s_timeout_ms) noexcept {
    const auto s_value = static_cast<std::uint8_t>(s_mode);
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout ||
        s_value < 1 || s_value > 4) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_control_mode = s_mode;
    return s_submit(RpcKind::kSetGripperControlMode, s_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::setArmMode(
    FciArmMode s_mode, std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout ||
        static_cast<std::uint8_t>(s_mode) > 4) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_arm_mode = s_mode;
    return s_submit(RpcKind::kSetArmMode, s_timeout_ms, s_request, false);
}

FciSubmitResult FciWirelinkEndpoint::home(
    std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    return s_submit(RpcKind::kHome, s_timeout_ms, OperationRequest{}, false);
}

FciSubmitResult FciWirelinkEndpoint::setZero(
    std::uint8_t s_joint_id, std::uint32_t s_timeout_ms) noexcept {
    if (s_joint_id > 6 || s_timeout_ms == 0 ||
        s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_joint_id = s_joint_id;
    return s_submit(RpcKind::kSetZero, s_timeout_ms, s_request, false);
}

FciSubmitResult FciWirelinkEndpoint::clearError(
    std::uint8_t s_joint_id, std::uint32_t s_timeout_ms) noexcept {
    if (s_joint_id > 6 || s_timeout_ms == 0 ||
        s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_joint_id = s_joint_id;
    return s_submit(RpcKind::kClearError, s_timeout_ms, s_request, false);
}

FciSubmitResult FciWirelinkEndpoint::clearFaults(
    std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    return s_submit(RpcKind::kClearFaults, s_timeout_ms,
                    OperationRequest{}, false);
}

FciSubmitResult FciWirelinkEndpoint::emergencyStop(
    std::uint32_t s_timeout_ms) noexcept {
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    return s_submit(RpcKind::kEmergencyStop, s_timeout_ms,
                    OperationRequest{}, false);
}

FciSubmitResult FciWirelinkEndpoint::readMotorRegister(
    std::uint8_t s_joint_id, std::uint8_t s_register_id,
    std::uint32_t s_timeout_ms) noexcept {
    if (s_joint_id > 6 || s_timeout_ms == 0 ||
        s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_joint_id = s_joint_id;
    s_request.m_register_id = s_register_id;
    return s_submit(RpcKind::kMotorRegisterRead, s_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::writeMotorRegister(
    std::uint8_t s_joint_id, std::uint8_t s_register_id, float s_value,
    std::uint32_t s_timeout_ms) noexcept {
    if (s_joint_id > 6 || !std::isfinite(s_value) || s_timeout_ms == 0 ||
        s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_joint_id = s_joint_id;
    s_request.m_register_id = s_register_id;
    s_request.m_value = s_value;
    return s_submit(RpcKind::kMotorRegisterWrite, s_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::storeMotorParameters(
    std::uint8_t s_joint_id, std::uint32_t s_timeout_ms) noexcept {
    if (s_joint_id > 6 || s_timeout_ms == 0 ||
        s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_joint_id = s_joint_id;
    return s_submit(RpcKind::kMotorStoreParameters, s_timeout_ms, s_request,
                    false);
}

FciSubmitResult FciWirelinkEndpoint::setMotorZero(
    std::uint8_t s_joint_id, std::uint32_t s_timeout_ms) noexcept {
    if (s_joint_id > 6 || s_timeout_ms == 0 ||
        s_timeout_ms >= s_kMaximumRelativeTimeout) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    s_request.m_joint_id = s_joint_id;
    return s_submit(RpcKind::kMotorSetZero, s_timeout_ms, s_request, false);
}

FciEndpointStatus FciWirelinkEndpoint::inspectOperation(
    std::uint64_t s_request_id, FciOperationResult& s_result_out) const
    noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    const std::size_t s_index = s_findSlot(s_request_id);
    if (s_index == s_kNoSlot) return FciEndpointStatus::kNoData;
    s_result_out = s_result(m_operations[s_index]);
    return s_result_out.m_status;
}

FciEndpointStatus FciWirelinkEndpoint::waitOperation(
    std::uint64_t s_request_id, std::chrono::milliseconds s_wait,
    FciOperationResult& s_result_out) const noexcept {
    if (s_wait.count() < 0) return FciEndpointStatus::kInvalidArgument;
    std::unique_lock<std::mutex> s_lock(m_mutex);
    auto s_index = s_findSlot(s_request_id);
    if (s_index == s_kNoSlot) return FciEndpointStatus::kNoData;
    const bool s_ready = m_operation_changed.wait_for(
        s_lock, s_wait, [&] {
            s_index = s_findSlot(s_request_id);
            return s_index == s_kNoSlot ||
                   s_terminal(m_operations[s_index].m_state);
        });
    if (s_index == s_kNoSlot) return FciEndpointStatus::kNoData;
    s_result_out = s_result(m_operations[s_index]);
    return s_ready ? s_result_out.m_status : FciEndpointStatus::kBusy;
}

FciEndpointStatus FciWirelinkEndpoint::takeOperation(
    std::uint64_t s_request_id, FciOperationResult& s_result_out) noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    const std::size_t s_index = s_findSlot(s_request_id);
    if (s_index == s_kNoSlot) return FciEndpointStatus::kNoData;
    if (!s_terminal(m_operations[s_index].m_state)) {
        s_result_out = s_result(m_operations[s_index]);
        return FciEndpointStatus::kBusy;
    }
    s_result_out = s_result(m_operations[s_index]);
    m_operations[s_index] = OperationSlot{};
    return s_result_out.m_status;
}

FciEndpointStatus FciWirelinkEndpoint::takeDeviceInfo(
    std::uint64_t s_request_id, FciOperationResult& s_result_out,
    DeviceInfo& s_info) noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    const std::size_t s_index = s_findSlot(s_request_id);
    if (s_index == s_kNoSlot ||
        m_operations[s_index].m_kind != RpcKind::kGetDeviceInfo) {
        return FciEndpointStatus::kNoData;
    }
    auto& s_slot = m_operations[s_index];
    if (!s_terminal(s_slot.m_state)) {
        s_result_out = s_result(s_slot);
        return FciEndpointStatus::kBusy;
    }
    s_result_out = s_result(s_slot);
    if (s_slot.m_status == FciEndpointStatus::kOk &&
        s_slot.m_device_info.m_valid) {
        s_info = DeviceInfo{
            .m_protocol_version = s_slot.m_device_info.m_protocol_version,
            .m_firmware_version = s_slot.m_device_info.m_firmware_version,
            .m_board_name = std::string(
                s_slot.m_device_info.m_board_name.data(),
                s_slot.m_device_info.m_board_name_size),
            .m_custom_name = std::string(
                s_slot.m_device_info.m_custom_name.data(),
                s_slot.m_device_info.m_custom_name_size),
            .m_firmware_type = s_slot.m_device_info.m_firmware_type,
            .m_serial_number = std::string(
                s_slot.m_device_info.m_serial_number.data(),
                s_slot.m_device_info.m_serial_number_size),
            .m_command_capabilities =
                s_slot.m_device_info.m_command_capabilities,
        };
    }
    s_slot = OperationSlot{};
    return s_result_out.m_status;
}

FciEndpointStatus FciWirelinkEndpoint::takeDeviceSettings(
    std::uint64_t s_request_id, FciOperationResult& s_result_out,
    DeviceSettings& s_settings) noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    const std::size_t s_index = s_findSlot(s_request_id);
    if (s_index == s_kNoSlot ||
        (m_operations[s_index].m_kind != RpcKind::kGetDeviceSettings &&
         m_operations[s_index].m_kind != RpcKind::kSetDeviceSettings)) {
        return FciEndpointStatus::kNoData;
    }
    auto& s_slot = m_operations[s_index];
    if (!s_terminal(s_slot.m_state)) {
        s_result_out = s_result(s_slot);
        return FciEndpointStatus::kBusy;
    }
    s_result_out = s_result(s_slot);
    if (s_slot.m_status == FciEndpointStatus::kOk &&
        s_slot.m_device_settings_valid) {
        s_settings = s_slot.m_device_settings;
    }
    s_slot = OperationSlot{};
    return s_result_out.m_status;
}

FciEndpointStatus FciWirelinkEndpoint::takeMotorRegister(
    std::uint64_t s_request_id, FciOperationResult& s_result_out,
    float& s_value) noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    const std::size_t s_index = s_findSlot(s_request_id);
    if (s_index == s_kNoSlot ||
        m_operations[s_index].m_kind != RpcKind::kMotorRegisterRead) {
        return FciEndpointStatus::kNoData;
    }
    auto& s_slot = m_operations[s_index];
    if (!s_terminal(s_slot.m_state)) {
        s_result_out = s_result(s_slot);
        return FciEndpointStatus::kBusy;
    }
    s_result_out = s_result(s_slot);
    if (s_slot.m_status == FciEndpointStatus::kOk &&
        s_slot.m_motor_register_valid) {
        s_value = s_slot.m_motor_register_value;
    }
    s_slot = OperationSlot{};
    return s_result_out.m_status;
}

FciControlLeaseSnapshot FciWirelinkEndpoint::controlLease() const noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    return m_lease;
}

FciEndpointStatus FciWirelinkEndpoint::sendJointMit(
    const JointMIT& s_command, std::uint32_t s_dt_us,
    std::uint64_t s_sdk_timestamp_us) noexcept {
    if (s_dt_us == 0 || !s_allFinite(s_command.m_q) ||
        !s_allFinite(s_command.m_dq) || !s_allFinite(s_command.m_tau) ||
        !s_allFinite(s_command.m_kp) || !s_allFinite(s_command.m_kd)) {
        return FciEndpointStatus::kInvalidArgument;
    }

    std::uint64_t s_lease_token{};
    const auto s_lease_status = s_commandToken(s_lease_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;

    joint_mit_command_t s_wire{};
    joint_mit_command_clear(&s_wire);
    s_wire.has_position = true;
    s_wire.has_velocity = true;
    s_wire.has_torque = true;
    s_wire.has_kp = true;
    s_wire.has_kd = true;
    std::copy_n(s_command.m_q, 6, s_wire.position);
    std::copy_n(s_command.m_dq, 6, s_wire.velocity);
    std::copy_n(s_command.m_tau, 6, s_wire.torque);
    std::copy_n(s_command.m_kp, 6, s_wire.kp);
    std::copy_n(s_command.m_kd, 6, s_wire.kd);
    s_wire.has_dt_us = true;
    s_wire.dt_us = s_dt_us;
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_gravity_compensation = true;
    s_wire.gravity_compensation = s_command.m_firmware_gravity;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_lease_token;

    std::array<std::uint8_t, WirelinkExecutor::s_kMaximumCommandPayload>
        s_payload{};
    std::size_t s_payload_size{};
    const wl_codec_status_t s_codec = joint_mit_command_encode(
        &s_wire, s_payload.data(), s_payload.size(), &s_payload_size);
    if (s_codec != WL_CODEC_OK) return FciEndpointStatus::kCodecError;
    return s_submitLatestPayload(JOINT_MIT_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_payload_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendGripperMit(
    const JointMIT& s_command, std::uint32_t s_dt_us,
    std::uint64_t s_sdk_timestamp_us) noexcept {
    if (s_dt_us == 0 || !std::isfinite(s_command.m_q[0]) ||
        !std::isfinite(s_command.m_dq[0]) ||
        !std::isfinite(s_command.m_tau[0]) ||
        !std::isfinite(s_command.m_kp[0]) ||
        !std::isfinite(s_command.m_kd[0])) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;

    gripper_mit_command_t s_wire{};
    gripper_mit_command_clear(&s_wire);
    s_wire.has_position = true;
    s_wire.position = s_command.m_q[0];
    s_wire.has_velocity = true;
    s_wire.velocity = s_command.m_dq[0];
    s_wire.has_torque = true;
    s_wire.torque = s_command.m_tau[0];
    s_wire.has_kp = true;
    s_wire.kp = s_command.m_kp[0];
    s_wire.has_kd = true;
    s_wire.kd = s_command.m_kd[0];
    s_wire.has_dt_us = true;
    s_wire.dt_us = s_dt_us;
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_gravity_compensation = true;
    s_wire.gravity_compensation = s_command.m_firmware_gravity;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;

    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (gripper_mit_command_encode(&s_wire, s_payload.data(),
                                   s_payload.size(), &s_size) != WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(GRIPPER_MIT_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendJointPositionVelocity(
    const JointPosVel& s_command,
    std::uint64_t s_sdk_timestamp_us) noexcept {
    if (!s_allFinite(s_command.m_q) || !s_allFinite(s_command.m_dq)) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    joint_position_velocity_command_t s_wire{};
    joint_position_velocity_command_clear(&s_wire);
    s_wire.has_position = true;
    s_wire.has_velocity = true;
    std::copy_n(s_command.m_q, 6, s_wire.position);
    std::copy_n(s_command.m_dq, 6, s_wire.velocity);
    s_wire.has_enabled_mask = true;
    s_wire.enabled_mask = UINT8_C(0x3f);
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (joint_position_velocity_command_encode(
            &s_wire, s_payload.data(), s_payload.size(), &s_size) !=
        WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(JOINT_POSITION_VELOCITY_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendJointVelocity(
    const JointVel& s_command, std::uint64_t s_sdk_timestamp_us) noexcept {
    if (!s_allFinite(s_command.m_dq)) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    joint_velocity_command_t s_wire{};
    joint_velocity_command_clear(&s_wire);
    s_wire.has_velocity = true;
    std::copy_n(s_command.m_dq, 6, s_wire.velocity);
    s_wire.has_enabled_mask = true;
    s_wire.enabled_mask = UINT8_C(0x3f);
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (joint_velocity_command_encode(&s_wire, s_payload.data(),
                                      s_payload.size(), &s_size) !=
        WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(JOINT_VELOCITY_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendJointPvt(
    const JointPVT& s_command, std::uint64_t s_sdk_timestamp_us) noexcept {
    if (!s_allFinite(s_command.m_q) ||
        !s_allFinite(s_command.m_dq_limit) ||
        !s_allFinite(s_command.m_current_limit_norm)) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    joint_pvt_command_t s_wire{};
    joint_pvt_command_clear(&s_wire);
    s_wire.has_position = true;
    s_wire.has_velocity_limit = true;
    s_wire.has_current_limit_normalized = true;
    std::copy_n(s_command.m_q, 6, s_wire.position);
    std::copy_n(s_command.m_dq_limit, 6, s_wire.velocity_limit);
    std::copy_n(s_command.m_current_limit_norm, 6,
                s_wire.current_limit_normalized);
    s_wire.has_enabled_mask = true;
    s_wire.enabled_mask = UINT8_C(0x3f);
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (joint_pvt_command_encode(&s_wire, s_payload.data(), s_payload.size(),
                                 &s_size) != WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(JOINT_PVT_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendCartesianPose(
    const CartesianPose& s_command, std::uint32_t s_dt_us,
    std::uint64_t s_sdk_timestamp_us) noexcept {
    if ((m_command_capabilities.load(std::memory_order_acquire) &
         commandCapability(CommandCapability::kCartesianPose)) == 0U) {
        return FciEndpointStatus::kUnsupported;
    }
    if (s_dt_us == 0 || !s_allFinite(s_command.m_T) ||
        !s_allFinite(s_command.m_kp) || !s_allFinite(s_command.m_kd)) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    cartesian_pose_command_t s_wire{};
    cartesian_pose_command_clear(&s_wire);
    s_wire.has_transform = true;
    s_wire.has_kp = true;
    s_wire.has_kd = true;
    std::copy_n(s_command.m_T, 16, s_wire.transform);
    std::copy_n(s_command.m_kp, 6, s_wire.kp);
    std::copy_n(s_command.m_kd, 6, s_wire.kd);
    s_wire.has_dt_us = true;
    s_wire.dt_us = s_dt_us;
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_gravity_compensation = true;
    s_wire.gravity_compensation = false;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (cartesian_pose_command_encode(&s_wire, s_payload.data(),
                                      s_payload.size(), &s_size) !=
        WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(CARTESIAN_POSE_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendCartesianVelocity(
    const CartesianVelocities& s_command, std::uint32_t s_dt_us,
    std::uint64_t s_sdk_timestamp_us) noexcept {
    if ((m_command_capabilities.load(std::memory_order_acquire) &
         commandCapability(CommandCapability::kCartesianVelocity)) == 0U) {
        return FciEndpointStatus::kUnsupported;
    }
    if (s_dt_us == 0 || !s_allFinite(s_command.m_v) ||
        !s_allFinite(s_command.m_kp) || !s_allFinite(s_command.m_kd)) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    cartesian_velocity_command_t s_wire{};
    cartesian_velocity_command_clear(&s_wire);
    s_wire.has_twist = true;
    s_wire.has_kp = true;
    s_wire.has_kd = true;
    std::copy_n(s_command.m_v, 6, s_wire.twist);
    std::copy_n(s_command.m_kp, 6, s_wire.kp);
    std::copy_n(s_command.m_kd, 6, s_wire.kd);
    s_wire.has_dt_us = true;
    s_wire.dt_us = s_dt_us;
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_gravity_compensation = true;
    s_wire.gravity_compensation = false;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (cartesian_velocity_command_encode(
            &s_wire, s_payload.data(), s_payload.size(), &s_size) !=
        WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(CARTESIAN_VELOCITY_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendGripperPositionVelocity(
    const JointPosVel& s_command,
    std::uint64_t s_sdk_timestamp_us) noexcept {
    if (!std::isfinite(s_command.m_q[0]) ||
        !std::isfinite(s_command.m_dq[0])) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    gripper_position_velocity_command_t s_wire{};
    gripper_position_velocity_command_clear(&s_wire);
    s_wire.has_position = true;
    s_wire.position = s_command.m_q[0];
    s_wire.has_velocity = true;
    s_wire.velocity = s_command.m_dq[0];
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (gripper_position_velocity_command_encode(
            &s_wire, s_payload.data(), s_payload.size(), &s_size) !=
        WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(
        GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID, s_payload.data(),
        s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendGripperVelocity(
    const JointVel& s_command, std::uint64_t s_sdk_timestamp_us) noexcept {
    if (!std::isfinite(s_command.m_dq[0])) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    gripper_velocity_command_t s_wire{};
    gripper_velocity_command_clear(&s_wire);
    s_wire.has_velocity = true;
    s_wire.velocity = s_command.m_dq[0];
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (gripper_velocity_command_encode(&s_wire, s_payload.data(),
                                        s_payload.size(), &s_size) !=
        WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(GRIPPER_VELOCITY_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

FciEndpointStatus FciWirelinkEndpoint::sendGripperPvt(
    const JointPVT& s_command, std::uint64_t s_sdk_timestamp_us) noexcept {
    if (!std::isfinite(s_command.m_q[0]) ||
        !std::isfinite(s_command.m_dq_limit[0]) ||
        !std::isfinite(s_command.m_current_limit_norm[0])) {
        return FciEndpointStatus::kInvalidArgument;
    }
    std::uint64_t s_token{};
    const auto s_lease_status = s_commandToken(s_token);
    if (s_lease_status != FciEndpointStatus::kOk) return s_lease_status;
    gripper_pvt_command_t s_wire{};
    gripper_pvt_command_clear(&s_wire);
    s_wire.has_position = true;
    s_wire.position = s_command.m_q[0];
    s_wire.has_velocity_limit = true;
    s_wire.velocity_limit = s_command.m_dq_limit[0];
    s_wire.has_current_limit_normalized = true;
    s_wire.current_limit_normalized = s_command.m_current_limit_norm[0];
    s_wire.has_sequence = true;
    s_wire.sequence =
        m_command_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    s_wire.has_sdk_timestamp_us = true;
    s_wire.sdk_timestamp_us = s_sdk_timestamp_us;
    s_wire.has_lease_token = true;
    s_wire.lease_token = s_token;
    std::array<std::uint8_t, s_kTxPayloadSize> s_payload{};
    std::size_t s_size{};
    if (gripper_pvt_command_encode(&s_wire, s_payload.data(),
                                   s_payload.size(), &s_size) != WL_CODEC_OK) {
        return FciEndpointStatus::kCodecError;
    }
    return s_submitLatestPayload(GRIPPER_PVT_COMMAND_MESSAGE_ID,
                                 s_payload.data(), s_size);
}

void FciWirelinkEndpoint::s_onEvent(void* s_user_data, wl_ctx_t*,
    const wl_event_t*, wl_time_ms_t) noexcept {
    static_cast<FciWirelinkEndpoint*>(s_user_data)->m_stats.m_dispatch_calls.fetch_add(
        1, std::memory_order_relaxed);
}

void FciWirelinkEndpoint::s_onResult(void* s_user_data,
    const fci_arm_runtime_result_t* s_result) noexcept {
    if (fci_arm_runtime_result_ok(s_result) || s_result->domain == FCI_ARM_RUNTIME_NON_RX) return;
    static_cast<FciWirelinkEndpoint*>(s_user_data)->m_stats.m_dispatch_errors.fetch_add(
        1, std::memory_order_relaxed);
}

uint8_t FciWirelinkEndpoint::s_applicationProgress(
    void* s_user_data, wl_ctx_t* s_context, wl_time_ms_t s_now_ms) noexcept {
    return static_cast<FciWirelinkEndpoint*>(s_user_data)->s_progress(*s_context, s_now_ms);
}

std::uint32_t FciWirelinkEndpoint::s_applicationDeadline(
    const void* s_user_data, wl_time_ms_t s_now_ms) noexcept {
    const auto& s_self = *static_cast<const FciWirelinkEndpoint*>(s_user_data);
    std::uint32_t s_nearest = WL_RPC_NO_DEADLINE_MS;

    std::lock_guard<std::mutex> s_lock(s_self.m_mutex);
    for (const auto& s_slot : s_self.m_operations) {
        if (s_slot.m_used && s_slot.m_state == FciOperationState::kQueued)
            s_nearest = std::min(s_nearest, s_queueRemaining(s_now_ms,
                s_slot.m_submitted_at_ms, s_slot.m_timeout_ms));
    }
    if (s_self.m_lease.m_state == FciControlLeaseState::kHeld) {
        s_nearest = std::min(
            s_nearest, s_until(s_now_ms, s_self.m_lease_renew_at_ms));
    } else if (s_self.m_lease.m_state ==
                   FciControlLeaseState::kRenewQueued ||
               s_self.m_lease.m_state == FciControlLeaseState::kRenewing) {
        s_nearest = std::min(
            s_nearest, s_until(s_now_ms, s_self.m_lease_expire_at_ms));
    }
    return s_nearest;
}

std::uint32_t FciWirelinkEndpoint::s_transportDeadline(
    const void* s_user_data, wl_time_ms_t s_now_ms) noexcept {
    const auto& s_self = *static_cast<const FciWirelinkEndpoint*>(s_user_data);
    return s_self.m_direct_transport == nullptr
               ? WL_POLL_NO_DEADLINE_MS
               : s_self.m_direct_transport->wirelinkDeadlineHint(s_now_ms);
}

void FciWirelinkEndpoint::s_quiesce(void* s_user_data) noexcept {
    auto& s_self = *static_cast<FciWirelinkEndpoint*>(s_user_data);
    if (s_self.m_direct_transport != nullptr) {
        s_self.m_direct_transport->quiesceWirelink();
    }
    s_self.s_stopOnOwner();
}

int FciWirelinkEndpoint::s_transportService(void* s_user_data) noexcept {
    auto& s_self = *static_cast<FciWirelinkEndpoint*>(s_user_data);
    return s_self.m_direct_transport == nullptr
               ? WL_OK
               : s_self.m_direct_transport->serviceWirelink();
}

void FciWirelinkEndpoint::s_transportWake(void* s_user_data) noexcept {
    if (s_user_data != nullptr) {
        static_cast<FciWirelinkEndpoint*>(s_user_data)->notify();
    }
}

wl_time_ms_t FciWirelinkEndpoint::s_nowMs() noexcept {
    using namespace std::chrono;
    return static_cast<wl_time_ms_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
            .count());
}

bool FciWirelinkEndpoint::s_terminal(FciOperationState s_state) noexcept {
    return s_state == FciOperationState::kCompleted ||
           s_state == FciOperationState::kTimedOut ||
           s_state == FciOperationState::kCancelled ||
           s_state == FciOperationState::kLinkFailed ||
           s_state == FciOperationState::kDomainError;
}

FciEndpointStatus FciWirelinkEndpoint::s_endpointStatus(int s_result) noexcept {
    switch (s_result) {
        case WL_OK: return FciEndpointStatus::kOk;
        case WL_ERR_QUEUE_FULL:
        case WL_ERR_NO_SPACE: return FciEndpointStatus::kQueueFull;
        case WL_ERR_BUSY:
        case WL_ERR_WOULD_BLOCK: return FciEndpointStatus::kBusy;
        case WL_ERR_INVALID_ARG: return FciEndpointStatus::kInvalidArgument;
        case WL_ERR_NOT_SUPPORTED: return FciEndpointStatus::kUnsupported;
        case WL_ERR_NO_DATA: return FciEndpointStatus::kNoData;
        case WL_ERR_CANCELLED: return FciEndpointStatus::kCancelled;
        case WL_ERR_NOT_INITIALIZED:
        case WL_ERR_INVALID_STATE: return FciEndpointStatus::kNotReady;
        default: return FciEndpointStatus::kLinkError;
    }
}

FciEndpointStatus FciWirelinkEndpoint::s_commandToken(
    std::uint64_t& s_token) const noexcept {
    s_token = 0;
    std::lock_guard<std::mutex> s_lock(m_mutex);
    if (!m_running) return FciEndpointStatus::kNotReady;
    if ((m_lease.m_state != FciControlLeaseState::kHeld &&
         m_lease.m_state != FciControlLeaseState::kRenewQueued &&
         m_lease.m_state != FciControlLeaseState::kRenewing) ||
        m_lease.m_token == 0) {
        return FciEndpointStatus::kNoLease;
    }
    s_token = m_lease.m_token;
    return FciEndpointStatus::kOk;
}

FciEndpointStatus FciWirelinkEndpoint::s_submitLatestPayload(
    std::uint16_t s_message_id, const std::uint8_t* s_payload,
    std::size_t s_payload_size) noexcept {
    return s_endpointStatus(
        m_executor.submitLatest(s_message_id, s_payload, s_payload_size));
}

std::uint32_t FciWirelinkEndpoint::s_until(wl_time_ms_t s_now,
                                           wl_time_ms_t s_target) noexcept {
    const std::int32_t s_remaining =
        static_cast<std::int32_t>(s_target - s_now);
    return s_remaining <= 0 ? 0 : static_cast<std::uint32_t>(s_remaining);
}

bool FciWirelinkEndpoint::s_progress(wl_ctx_t& s_context,
                                     wl_time_ms_t s_now_ms) noexcept {
    m_stats.m_runtime_poll_calls.fetch_add(1, std::memory_order_relaxed);

    // A serialized business queue must not extend a caller's RPC deadline.
    for (std::size_t s_index = 0; s_index < m_operations.size(); ++s_index) {
        bool s_expired;
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            const auto& s_slot = m_operations[s_index];
            s_expired = s_slot.m_used && s_slot.m_state == FciOperationState::kQueued &&
                s_queueRemaining(s_now_ms, s_slot.m_submitted_at_ms,
                    s_slot.m_timeout_ms) == 0U;
        }
        if (s_expired) {
            wl_rpc_completion_t s_timeout{};
            s_timeout.status = WL_RPC_TIMED_OUT;
            s_finalize(s_index, s_timeout, static_cast<const home_response_value_t*>(nullptr));
        }
    }
    (void)s_drainLatest();
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if ((m_lease.m_state == FciControlLeaseState::kRenewQueued ||
             m_lease.m_state == FciControlLeaseState::kRenewing) &&
            s_until(s_now_ms, m_lease_expire_at_ms) == 0) {
            m_lease.m_state = FciControlLeaseState::kExpired;
            m_lease.m_status = FciEndpointStatus::kTimeout;
            m_lease.m_token = 0;
            m_lease.m_granted_timeout_ms = 0;
        }
    }
    (void)s_scheduleRenewal(s_now_ms);
    // Consumed telemetry/completions and expired leases are history, not a
    // request for another owner pass. Core/adapter hints and producer notify()
    // cover remaining RX, deadlines and callbacks that enqueue work. A newly
    // started RPC still needs one pass to service its TX or finish a local
    // terminal result; backpressure must wait for readiness instead of spin.
    return s_startNext(s_context, s_now_ms);
}

bool FciWirelinkEndpoint::s_drainLatest() noexcept {
    bool s_progressed = false;
    fci_arm_arm_status_latest_view_t s_arm_view{};
    const int s_arm_result = fci_arm_arm_status_latest_acquire(
        s_runtime(), &s_arm_view);
    if (s_arm_result == WL_OK) {
        m_stats.m_latest_acquires.fetch_add(1, std::memory_order_relaxed);
        if (s_arm_view.value != nullptr && s_validArmStatus(*s_arm_view.value)) {
            const auto s_snapshot =
                s_armStatusFromWire(*s_arm_view.value, s_arm_view.generation);
            const auto s_callback = m_arm_status_callback;
            void* const s_user_data = m_callback_user_data;
            if (s_callback != nullptr) s_callback(s_user_data, s_snapshot);
        }
        if (fci_arm_arm_status_latest_release(s_runtime(),
                                              &s_arm_view) == WL_OK) {
            m_stats.m_latest_releases.fetch_add(1,
                                               std::memory_order_relaxed);
        }
        s_progressed = true;
    }

    fci_arm_arm_diagnostics_latest_view_t s_diagnostics_view{};
    const int s_diagnostics_result = fci_arm_arm_diagnostics_latest_acquire(
        s_runtime(), &s_diagnostics_view);
    if (s_diagnostics_result == WL_OK) {
        m_stats.m_latest_acquires.fetch_add(1, std::memory_order_relaxed);
        ArmDiagnostics s_domain{};
        if (s_diagnostics_view.value != nullptr &&
            s_diagnosticsFromWire(*s_diagnostics_view.value, s_domain)) {
            const auto s_callback = m_diagnostics_callback;
            void* const s_user_data = m_callback_user_data;
            if (s_callback != nullptr) s_callback(s_user_data, s_domain);
        }
        if (fci_arm_arm_diagnostics_latest_release(
                s_runtime(), &s_diagnostics_view) == WL_OK) {
            m_stats.m_latest_releases.fetch_add(1,
                                               std::memory_order_relaxed);
        }
        s_progressed = true;
    }
    return s_progressed;
}

template <typename Response>
void FciWirelinkEndpoint::s_complete(void* s_context,
    const wl_rpc_completion_t* s_completion, const Response* s_response) noexcept {
    auto& s_slot = *static_cast<OperationSlot*>(s_context);
    auto& s_self = *s_slot.m_owner;
    s_self.s_finalize(static_cast<std::size_t>(&s_slot - s_self.m_operations.data()),
                      *s_completion, s_response);
}

template <typename Response>
void FciWirelinkEndpoint::s_finalize(std::size_t s_index,
    const wl_rpc_completion_t& s_completion, const Response* s_response_ptr) noexcept {
    wl_time_ms_t s_now_ms{};
    (void)wl_endpoint_now(fci_arm_endpoint_handle(&m_endpoint), &s_now_ms);
    RpcKind s_kind{};
    bool s_internal{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        s_kind = m_operations[s_index].m_kind;
        s_internal = m_operations[s_index].m_internal;
    }
    FciOperationState s_state = FciOperationState::kLinkFailed;
    switch (s_completion.status) {
        case WL_RPC_SUCCESS: s_state = FciOperationState::kCompleted; break;
        case WL_RPC_REJECTED: s_state = FciOperationState::kDomainError; break;
        case WL_RPC_TIMED_OUT: s_state = FciOperationState::kTimedOut; break;
        case WL_RPC_CANCELLED: s_state = FciOperationState::kCancelled; break;
        default: break;
    }
    if (s_state == FciOperationState::kTimedOut)
        m_stats.m_runtime_timeouts.fetch_add(1, std::memory_order_relaxed);
    if (s_state == FciOperationState::kCancelled)
        m_stats.m_rpc_cancelled.fetch_add(1, std::memory_order_relaxed);
    FciEndpointStatus s_status{FciEndpointStatus::kInternalError};
    std::int32_t s_domain_status = s_completion.rejection;
    FixedDeviceInfo s_device_info{};
    DeviceSettings s_device_settings{};
    float s_motor_register_value{};
    bool s_device_settings_valid{}, s_motor_register_valid{};
    std::uint64_t s_granted_token{};
    std::uint32_t s_granted_timeout{};
    bool s_response_valid = true;
    if (s_completion.status == WL_RPC_SUCCESS) {
        s_response_valid = s_response_ptr != nullptr;
        if (s_response_ptr != nullptr) {
            const auto& s_response = *s_response_ptr;
            if constexpr (std::is_same_v<Response, acquire_control_lease_response_value_t>) {
                s_response_valid = s_response.has_lease_token && s_response.lease_token != 0U &&
                    s_response.has_granted_timeout_ms && s_response.granted_timeout_ms != 0U &&
                    s_response.granted_timeout_ms < UINT32_C(0x80000000);
                s_granted_token = s_response.lease_token;
                s_granted_timeout = s_response.granted_timeout_ms;
            } else if constexpr (std::is_same_v<Response, get_device_info_response_value_t>) {
                const auto& s_info = s_response.info;
                s_response_valid = s_response.has_info && s_info.serial.length != 0U;
                if (s_response_valid) {
                    s_device_info.m_protocol_version = {s_info.protocol_version.major,
                        s_info.protocol_version.minor, s_info.protocol_version.patch};
                    s_device_info.m_firmware_version = {s_info.firmware_version.major,
                        s_info.firmware_version.minor, s_info.firmware_version.patch};
                    s_device_info.m_firmware_type = s_firmwareType(s_info.firmware_type);
                    s_device_info.m_command_capabilities = s_info.has_command_capabilities
                        ? s_info.command_capabilities : 0U;
                    s_response_valid = s_copyString(s_info.board_name,
                        s_device_info.m_board_name.data(), s_device_info.m_board_name.size(),
                        s_device_info.m_board_name_size) &&
                        s_copyString(s_info.custom_name, s_device_info.m_custom_name.data(),
                            s_device_info.m_custom_name.size(), s_device_info.m_custom_name_size) &&
                        s_copyString(s_info.serial, s_device_info.m_serial_number.data(),
                            s_device_info.m_serial_number.size(), s_device_info.m_serial_number_size);
                    s_device_info.m_valid = s_response_valid;
                }
            } else if constexpr (std::is_same_v<Response, get_device_settings_response_value_t> ||
                                 std::is_same_v<Response, set_device_settings_response_value_t>) {
                s_response_valid = s_response.has_settings &&
                    s_settingsFromWire(s_response.settings, s_device_settings);
                s_device_settings_valid = s_response_valid;
            } else if constexpr (std::is_same_v<Response, motor_register_read_response_value_t>) {
                OperationRequest s_request{};
                {
                    std::lock_guard<std::mutex> s_lock(m_mutex);
                    s_request = m_operations[s_index].m_request;
                }
                s_response_valid = s_response.has_joint_id &&
                    s_response.joint_id == s_request.m_joint_id && s_response.has_register_id &&
                    s_response.register_id == s_request.m_register_id && s_response.has_value &&
                    std::isfinite(s_response.value);
                s_motor_register_value = s_response.value;
                s_motor_register_valid = s_response_valid;
            }
        }
    }

    if (!s_response_valid) {
        s_status = FciEndpointStatus::kInternalError;
    } else {
        switch (s_state) {
            case FciOperationState::kCompleted:
                s_status = s_domain_status == 0
                               ? FciEndpointStatus::kOk
                               : FciEndpointStatus::kDomainError;
                break;
            case FciOperationState::kDomainError:
                s_status = FciEndpointStatus::kDomainError;
                break;
            case FciOperationState::kTimedOut:
                s_status = FciEndpointStatus::kTimeout;
                break;
            case FciOperationState::kCancelled:
                s_status = FciEndpointStatus::kCancelled;
                break;
            case FciOperationState::kLinkFailed:
                s_status = FciEndpointStatus::kLinkError;
                break;
            default:
                s_status = FciEndpointStatus::kInternalError;
                break;
        }
    }

    // Generated async releases admitted slots before delivering this callback.
    if (m_operations[s_index].m_admitted)
        m_stats.m_rpc_released.fetch_add(1, std::memory_order_relaxed);

    if (s_kind == RpcKind::kGetDeviceInfo &&
        s_status == FciEndpointStatus::kOk && s_device_info.m_valid) {
        m_command_capabilities.store(s_device_info.m_command_capabilities,
                                     std::memory_order_release);
    }

    std::lock_guard<std::mutex> s_lock(m_mutex);
    auto& s_slot = m_operations[s_index];
    s_slot.m_state = s_state;
    s_slot.m_status = s_status;
    s_slot.m_domain_status = s_domain_status;
    s_slot.m_link_status = s_completion.transport_error;
    s_slot.m_device_info = s_device_info;
    s_slot.m_device_settings = s_device_settings;
    s_slot.m_motor_register_value = s_motor_register_value;
    s_slot.m_device_settings_valid = s_device_settings_valid;
    s_slot.m_motor_register_valid = s_motor_register_valid;

    if (!m_running) {
        m_lease = FciControlLeaseSnapshot{};
        m_lease.m_status = FciEndpointStatus::kCancelled;
    } else if (s_kind == RpcKind::kAcquireLease) {
        if (s_status == FciEndpointStatus::kOk) {
            m_lease = FciControlLeaseSnapshot{
                .m_state = FciControlLeaseState::kHeld,
                .m_status = FciEndpointStatus::kOk,
                .m_domain_status = s_domain_status,
                .m_token = s_granted_token,
                .m_granted_timeout_ms = s_granted_timeout,
            };
            m_lease_requested_timeout_ms =
                s_slot.m_request.m_requested_lease_timeout_ms;
            const std::uint32_t s_renew_delay =
                std::max<std::uint32_t>(1, s_granted_timeout / 2);
            m_lease_renew_at_ms = s_now_ms + s_renew_delay;
            m_lease_expire_at_ms = s_now_ms + s_granted_timeout;
        } else {
            m_lease.m_state =
                s_state == FciOperationState::kTimedOut
                    ? FciControlLeaseState::kExpired
                    : FciControlLeaseState::kFailed;
            m_lease.m_status = s_status;
            m_lease.m_domain_status = s_domain_status;
            m_lease.m_token = 0;
            m_lease.m_granted_timeout_ms = 0;
        }
    } else if (s_kind == RpcKind::kReleaseLease) {
        if (s_status == FciEndpointStatus::kOk) {
            m_lease = FciControlLeaseSnapshot{};
        } else {
            m_lease.m_state = FciControlLeaseState::kFailed;
            m_lease.m_status = s_status;
            m_lease.m_domain_status = s_domain_status;
            m_lease.m_token = 0;
            m_lease.m_granted_timeout_ms = 0;
        }
    }

    if (m_active_operation == s_index) m_active_operation = s_kNoSlot;
    if (s_internal) s_slot = OperationSlot{};
    m_operation_changed.notify_all();
    m_executor.notify(); // The next serialized product operation can start.
}


bool FciWirelinkEndpoint::s_startNext(wl_ctx_t& s_context,
                                      wl_time_ms_t s_now_ms) noexcept {
    std::size_t s_index{};
    OperationSlot s_slot{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (m_active_operation != s_kNoSlot) return false;
        s_index = s_findQueuedSlot();
        if (s_index == s_kNoSlot) return false;
        m_active_operation = s_index;
        m_operations[s_index].m_state = FciOperationState::kLinkPending;
        s_slot = m_operations[s_index];
        if (s_slot.m_kind == RpcKind::kAcquireLease) {
            m_lease.m_state =
                (s_slot.m_internal || s_slot.m_request.m_lease_token != 0)
                                  ? FciControlLeaseState::kRenewing
                                  : FciControlLeaseState::kAcquiring;
        } else if (s_slot.m_kind == RpcKind::kReleaseLease) {
            m_lease.m_state = FciControlLeaseState::kReleasing;
        }
    }

    int s_started = WL_ERR_INVALID_ARG;
    const auto s_remaining = s_queueRemaining(s_now_ms, s_slot.m_submitted_at_ms,
                                             s_slot.m_timeout_ms);
    if (s_remaining == 0U) {
        wl_rpc_completion_t s_timeout{};
        s_timeout.status = WL_RPC_TIMED_OUT;
        s_finalize(s_index, s_timeout, static_cast<const home_response_value_t*>(nullptr));
        return true;
    }
    (void)s_context;
    switch (s_slot.m_kind) {
        case RpcKind::kAcquireLease: {
            acquire_control_lease_request_value_t s_request{};
            s_request.has_requested_timeout_ms = true;
            s_request.requested_timeout_ms =
                s_slot.m_request.m_requested_lease_timeout_ms;
            if (s_slot.m_request.m_lease_token != 0) {
                s_request.has_current_token = true;
                s_request.current_token = s_slot.m_request.m_lease_token;
            }
            s_started = fci_arm_endpoint_acquire_control_lease_async(
                &m_endpoint, &s_request, s_remaining, s_complete<acquire_control_lease_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kReleaseLease: {
            release_control_lease_request_value_t s_request{};
            s_request.has_lease_token = true;
            s_request.lease_token = s_slot.m_request.m_lease_token;
            s_started = fci_arm_endpoint_release_control_lease_async(
                &m_endpoint, &s_request, s_remaining, s_complete<release_control_lease_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kGetDeviceInfo: {
            get_device_info_request_value_t s_request{};
            s_started = fci_arm_endpoint_get_device_info_async(
                &m_endpoint, &s_request, s_remaining, s_complete<get_device_info_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kSetDeviceInfo: {
            set_device_info_request_value_t s_request{};
            s_request.has_custom_name = true;
            s_request.custom_name.length = s_slot.m_request.m_custom_name_size;
            std::copy_n(s_slot.m_request.m_custom_name.data(), s_request.custom_name.length,
                        s_request.custom_name.data);
            s_started = fci_arm_endpoint_set_device_info_async(
                &m_endpoint, &s_request, s_remaining, s_complete<set_device_info_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kGetDeviceSettings: {
            get_device_settings_request_value_t s_request{};
            s_started = fci_arm_endpoint_get_device_settings_async(
                &m_endpoint, &s_request, s_remaining, s_complete<get_device_settings_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kSetDeviceSettings: {
            set_device_settings_request_value_t s_request{};
            s_request.has_settings = true;
            s_settingsToWire(s_slot.m_request.m_device_settings,
                             s_request.settings);
            s_started = fci_arm_endpoint_set_device_settings_async(
                &m_endpoint, &s_request, s_remaining, s_complete<set_device_settings_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kSetArmControlMode: {
            set_arm_control_mode_request_value_t s_request{};
            s_request.has_mode = true;
            s_request.mode = s_controlMode(s_slot.m_request.m_control_mode);
            s_started = fci_arm_endpoint_set_arm_control_mode_async(
                &m_endpoint, &s_request, s_remaining, s_complete<set_arm_control_mode_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kSetGripperControlMode: {
            set_gripper_control_mode_request_value_t s_request{};
            s_request.has_mode = true;
            s_request.mode = s_controlMode(s_slot.m_request.m_control_mode);
            s_started = fci_arm_endpoint_set_gripper_control_mode_async(
                &m_endpoint, &s_request, s_remaining, s_complete<set_gripper_control_mode_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kSetArmMode: {
            set_arm_mode_request_value_t s_request{};
            s_request.has_mode = true;
            s_request.mode = s_armMode(s_slot.m_request.m_arm_mode);
            s_started = fci_arm_endpoint_set_arm_mode_async(
                &m_endpoint, &s_request, s_remaining, s_complete<set_arm_mode_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kHome: {
            home_request_value_t s_request{};
            s_started = fci_arm_endpoint_home_async(
                &m_endpoint, &s_request, s_remaining, s_complete<home_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kSetZero: {
            set_zero_request_value_t s_request{};
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started = fci_arm_endpoint_set_zero_async(
                &m_endpoint, &s_request, s_remaining, s_complete<set_zero_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kClearError: {
            clear_error_request_value_t s_request{};
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started = fci_arm_endpoint_clear_error_async(
                &m_endpoint, &s_request, s_remaining, s_complete<clear_error_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kClearFaults: {
            clear_faults_request_value_t s_request{};
            s_started = fci_arm_endpoint_clear_faults_async(
                &m_endpoint, &s_request, s_remaining, s_complete<clear_faults_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kEmergencyStop: {
            emergency_stop_request_value_t s_request{};
            s_started = fci_arm_endpoint_emergency_stop_async(
                &m_endpoint, &s_request, s_remaining, s_complete<emergency_stop_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kMotorRegisterRead: {
            motor_register_read_request_value_t s_request{};
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_request.has_register_id = true;
            s_request.register_id = s_slot.m_request.m_register_id;
            s_started = fci_arm_endpoint_motor_register_read_async(
                &m_endpoint, &s_request, s_remaining, s_complete<motor_register_read_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kMotorRegisterWrite: {
            motor_register_write_request_value_t s_request{};
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_request.has_register_id = true;
            s_request.register_id = s_slot.m_request.m_register_id;
            s_request.has_value = true;
            s_request.value = s_slot.m_request.m_value;
            s_started = fci_arm_endpoint_motor_register_write_async(
                &m_endpoint, &s_request, s_remaining, s_complete<motor_register_write_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kMotorStoreParameters: {
            motor_store_parameters_request_value_t s_request{};
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started =
                fci_arm_endpoint_motor_store_parameters_async(
                &m_endpoint, &s_request, s_remaining, s_complete<motor_store_parameters_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kMotorSetZero: {
            motor_set_zero_request_value_t s_request{};
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started = fci_arm_endpoint_motor_set_zero_async(
                &m_endpoint, &s_request, s_remaining, s_complete<motor_set_zero_response_value_t>,
                &m_operations[s_index], nullptr);
            break;
        }
        case RpcKind::kNone:
            break;
    }

    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        auto& s_operation = m_operations[s_index];
        if (s_started == WL_OK) {
            s_operation.m_admitted = true;
            m_stats.m_rpc_started.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        m_active_operation = s_kNoSlot;
        // Admission may be retried; elapsed queue time stays in the deadline.
        s_operation.m_state = FciOperationState::kQueued;
    }
    if (s_started != WL_ERR_BUSY && s_started != WL_ERR_WOULD_BLOCK) {
        wl_rpc_completion_t s_failed{};
        s_failed.status = WL_RPC_FAILED;
        s_failed.local_error = s_started;
        s_finalize(s_index, s_failed, static_cast<const home_response_value_t*>(nullptr));
    }
    return false;
}

bool FciWirelinkEndpoint::s_scheduleRenewal(wl_time_ms_t s_now_ms) noexcept {
    std::uint64_t s_token{};
    std::uint32_t s_timeout{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (m_lease.m_state != FciControlLeaseState::kHeld ||
            m_lease.m_token == 0 ||
            s_until(s_now_ms, m_lease_renew_at_ms) != 0) {
            return false;
        }
        s_token = m_lease.m_token;
        s_timeout = m_lease_requested_timeout_ms;
    }
    OperationRequest s_request{};
    s_request.m_requested_lease_timeout_ms = s_timeout;
    s_request.m_lease_token = s_token;
    const auto s_result = s_submit(RpcKind::kAcquireLease, s_timeout,
                                   s_request, true);
    return s_result.m_status == FciEndpointStatus::kOk;
}

void FciWirelinkEndpoint::s_stopOnOwner() noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    m_running = false;
    for (auto& s_slot : m_operations) {
        if (!s_slot.m_used || s_terminal(s_slot.m_state) ||
            s_slot.m_state != FciOperationState::kQueued) continue;
        s_slot.m_state = FciOperationState::kCancelled;
        s_slot.m_status = FciEndpointStatus::kCancelled;
        if (s_slot.m_internal) s_slot = OperationSlot{};
    }
    m_lease = FciControlLeaseSnapshot{};
    m_lease.m_status = FciEndpointStatus::kCancelled;
    m_operation_changed.notify_all();
}

FciSubmitResult FciWirelinkEndpoint::s_submit(
    RpcKind s_kind, std::uint32_t s_timeout_ms,
    const OperationRequest& s_request, bool s_internal) noexcept {
    std::uint64_t s_request_id{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (!m_running) return {FciEndpointStatus::kNotReady, 0};
        const std::size_t s_index = s_allocateSlot(s_internal);
        if (s_index == s_kNoSlot) {
            return {FciEndpointStatus::kQueueFull, 0};
        }
        s_request_id = m_next_request_id++;
        if (m_next_request_id == 0) m_next_request_id = 1;
        m_operations[s_index] = OperationSlot{
            .m_owner = this,
            .m_request_id = s_request_id,
            .m_timeout_ms = s_timeout_ms,
            .m_submitted_at_ms = s_nowMs(),
            .m_request = s_request,
            .m_kind = s_kind,
            .m_state = FciOperationState::kQueued,
            .m_status = FciEndpointStatus::kOk,
            .m_internal = s_internal,
            .m_used = true,
        };
        if (s_kind == RpcKind::kAcquireLease) {
            m_lease.m_state =
                (s_internal || s_request.m_lease_token != 0)
                                  ? FciControlLeaseState::kRenewQueued
                                  : FciControlLeaseState::kAcquireQueued;
            m_lease.m_status = FciEndpointStatus::kOk;
        } else if (s_kind == RpcKind::kReleaseLease) {
            m_lease.m_state = FciControlLeaseState::kReleaseQueued;
            m_lease.m_status = FciEndpointStatus::kOk;
        }
    }
    m_executor.notify();
    return {FciEndpointStatus::kOk, s_request_id};
}

std::size_t FciWirelinkEndpoint::s_findSlot(
    std::uint64_t s_request_id) const noexcept {
    for (std::size_t s_index = 0; s_index < m_operations.size(); ++s_index) {
        if (m_operations[s_index].m_used &&
            m_operations[s_index].m_request_id == s_request_id) {
            return s_index;
        }
    }
    return s_kNoSlot;
}

std::size_t FciWirelinkEndpoint::s_findQueuedSlot() const noexcept {
    std::size_t s_selected = s_kNoSlot;
    std::uint64_t s_oldest = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t s_index = 0; s_index < m_operations.size(); ++s_index) {
        const auto& s_slot = m_operations[s_index];
        if (s_slot.m_used && s_slot.m_state == FciOperationState::kQueued &&
            s_slot.m_request_id < s_oldest) {
            s_selected = s_index;
            s_oldest = s_slot.m_request_id;
        }
    }
    return s_selected;
}

std::size_t FciWirelinkEndpoint::s_allocateSlot(bool s_internal) const
    noexcept {
    if (s_internal && !m_operations.back().m_used) {
        return m_operations.size() - 1;
    }
    const std::size_t s_limit =
        s_internal ? m_operations.size() : s_kPublicOperationCapacity;
    for (std::size_t s_index = 0; s_index < s_limit; ++s_index) {
        if (!m_operations[s_index].m_used) return s_index;
    }
    return s_kNoSlot;
}

FciOperationResult FciWirelinkEndpoint::s_result(
    const OperationSlot& s_slot) const noexcept {
    return FciOperationResult{
        .m_request_id = s_slot.m_request_id,
        .m_state = s_slot.m_state,
        .m_status = s_slot.m_status,
        .m_domain_status = s_slot.m_domain_status,
        .m_link_status = s_slot.m_link_status,
    };
}

FciWirelinkEndpointStats FciWirelinkEndpoint::stats() const noexcept {
    return FciWirelinkEndpointStats{
        .m_dispatch_calls =
            m_stats.m_dispatch_calls.load(std::memory_order_relaxed),
        .m_dispatch_errors =
            m_stats.m_dispatch_errors.load(std::memory_order_relaxed),
        .m_runtime_poll_calls =
            m_stats.m_runtime_poll_calls.load(std::memory_order_relaxed),
        .m_runtime_timeouts =
            m_stats.m_runtime_timeouts.load(std::memory_order_relaxed),
        .m_latest_acquires =
            m_stats.m_latest_acquires.load(std::memory_order_relaxed),
        .m_latest_releases =
            m_stats.m_latest_releases.load(std::memory_order_relaxed),
        .m_rpc_started = m_stats.m_rpc_started.load(std::memory_order_relaxed),
        .m_rpc_released =
            m_stats.m_rpc_released.load(std::memory_order_relaxed),
        .m_rpc_cancelled =
            m_stats.m_rpc_cancelled.load(std::memory_order_relaxed),
        .m_endpoint_storage_bytes = m_endpoint_storage_bytes,
    };
}

} // namespace florid::detail
