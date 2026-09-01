#include "florid/detail/FciWirelinkEndpoint.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <random>

namespace florid::detail {

namespace {

constexpr std::size_t s_kNoSlot = FciWirelinkEndpoint::s_kOperationCapacity;
constexpr std::uint32_t s_kMaximumRelativeTimeout = UINT32_C(0x7fffffff);

std::uint64_t s_mixSessionEntropy(std::uint64_t s_value) noexcept {
    s_value += UINT64_C(0x9e3779b97f4a7c15);
    s_value = (s_value ^ (s_value >> 30U)) *
              UINT64_C(0xbf58476d1ce4e5b9);
    s_value = (s_value ^ (s_value >> 27U)) *
              UINT64_C(0x94d049bb133111eb);
    return s_value ^ (s_value >> 31U);
}

std::uint64_t s_makeSessionId(const void* s_instance) noexcept {
    static std::atomic<std::uint64_t> s_counter{1};
    const auto s_wall_count = static_cast<std::uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    const auto s_steady_count = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::uint64_t s_entropy =
        s_wall_count ^ s_steady_count ^
        static_cast<std::uint64_t>(
            reinterpret_cast<std::uintptr_t>(s_instance)) ^
        s_counter.fetch_add(1, std::memory_order_relaxed);
    try {
        std::random_device s_random;
        s_entropy ^= static_cast<std::uint64_t>(s_random()) << 32U;
        s_entropy ^= static_cast<std::uint64_t>(s_random());
    } catch (...) {
        // Independent clocks, ASLR, and the process-local counter remain a
        // useful fallback on platforms without a usable random_device.
    }
    const std::uint64_t s_session_id = s_mixSessionEntropy(s_entropy);
    return s_session_id == 0U ? UINT64_C(1) : s_session_id;
}

std::uint32_t s_rpcOperationSeed(std::uint64_t s_session_id) noexcept {
    const std::uint64_t s_mixed = s_mixSessionEntropy(
        s_session_id ^ UINT64_C(0x5250432d4f504944));
    const std::uint32_t s_seed = static_cast<std::uint32_t>(s_mixed) ^
                                 static_cast<std::uint32_t>(s_mixed >> 32U);
    return s_seed == 0U ? UINT32_C(1) : s_seed;
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
        !s_wire.has_gripper_temperature_c ||
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
        };
    }
    s_domain.m_gripper = GripperDiagnostics{
        .m_healthy = s_wire.gripper_healthy,
        .m_temperature_c = s_wire.gripper_temperature_c,
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

bool s_firmwareTypeToWire(FirmwareType s_type,
                          firmware_type_t& s_wire) noexcept {
    switch (s_type) {
        case FirmwareType::kStandardArm:
            s_wire = FIRMWARE_STANDARD_ARM;
            return true;
        case FirmwareType::kMobileArm:
            s_wire = FIRMWARE_MOBILE_ARM;
            return true;
        case FirmwareType::kCobotArm:
            s_wire = FIRMWARE_COBOT_ARM;
            return true;
        case FirmwareType::kUnknown:
            return false;
    }
    return false;
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
            s_limits.m_min > s_limits.m_max) {
            return false;
        }
    }
    return true;
}

bool s_settingsFromWire(const device_settings_t& s_wire,
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

void s_settingsToWire(const DeviceSettings& s_settings,
                      device_settings_t& s_wire) noexcept {
    device_settings_clear(&s_wire);
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

bool s_copyString(const wl_codec_string_t& s_source, char* s_destination,
                  std::size_t s_capacity,
                  std::uint8_t& s_size) noexcept {
    if (s_source.length >= s_capacity ||
        (s_source.length != 0 && s_source.data == nullptr)) {
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

    const std::uint64_t s_session_id =
        s_config.m_session_id == 0U ? s_makeSessionId(this)
                                    : s_config.m_session_id;

    fci_arm_runtime_config_t s_runtime_config{};
    s_runtime_config.arm_status_latest_initial_generation = 1;
    s_runtime_config.motor_feedback_latest_initial_generation = 1;
    s_runtime_config.arm_diagnostics_latest_initial_generation = 1;
    s_runtime_config.rpc_client_enabled = 1;
    s_runtime_config.rpc_client_slot_count = s_kOperationCapacity;
    s_runtime_config.rpc_client_response_capacity = s_kTxPayloadSize;
    s_runtime_config.rpc_client_next_operation_id =
        s_rpcOperationSeed(s_session_id);

    fci_arm_runtime_requirements_t s_requirements{};
    int s_result =
        fci_arm_runtime_requirements(&s_runtime_config, &s_requirements);
    if (s_result != WL_OK ||
        s_requirements.storage_size > m_runtime_storage.size() ||
        s_requirements.storage_alignment > alignof(std::max_align_t)) {
        return s_result == WL_OK ? FciEndpointStatus::kInternalError
                                 : s_endpointStatus(s_result);
    }
    const fci_arm_runtime_storage_t s_runtime_storage{
        .data = m_runtime_storage.data(),
        .size = m_runtime_storage.size(),
    };
    s_result = fci_arm_runtime_init(&m_runtime_instance, &s_runtime_config,
                                    &s_runtime_storage);
    if (s_result != WL_OK) return s_endpointStatus(s_result);
    m_runtime_storage_bytes = s_requirements.storage_size;

    const wl_config_t s_link_config{
        .max_payload_len = s_kTxPayloadSize,
        .envelope = WL_ENVELOPE_COBS_STREAM,
        .integrity = WL_INTEGRITY_NONE,
        .session_id = s_session_id,
        .max_retries = s_config.m_max_retries,
        .ack_timeout_ms = s_config.m_ack_timeout_ms,
        .max_transmission_unit = s_kTxUnitSize,
    };
    const wl_storage_t s_link_storage{
        .tx_payload = m_tx_payload.data(),
        .tx_payload_size = m_tx_payload.size(),
        .tx_unit = m_tx_unit.data(),
        .tx_unit_size = m_tx_unit.size(),
        .control_unit = m_control_unit.data(),
        .control_unit_size = m_control_unit.size(),
        .rx_fifo = m_rx_fifo.data(),
        .rx_fifo_size = m_rx_fifo.size(),
        .rx_fallback = m_rx_fallback.data(),
        .rx_fallback_size = m_rx_fallback.size(),
    };
    s_result = m_executor.initialize(s_link_config, s_link_storage);
    if (s_result != WL_OK) return s_endpointStatus(s_result);

    WirelinkExecutorHooks s_hooks{};
    s_hooks.m_user_data = this;
    s_hooks.m_application_progress = s_applicationProgress;
    s_hooks.m_application_deadline_hint = s_applicationDeadline;
    s_hooks.m_on_event = s_onEvent;
    s_hooks.m_quiesce = s_quiesce;
    s_result = m_executor.setHooks(s_hooks);
    if (s_result != WL_OK) return s_endpointStatus(s_result);

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
    }
    const int s_result = m_executor.start();
    if (s_result != WL_OK) return s_endpointStatus(s_result);
    std::lock_guard<std::mutex> s_lock(m_mutex);
    m_running = true;
    return FciEndpointStatus::kOk;
}

void FciWirelinkEndpoint::stop() noexcept {
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (!m_initialized) return;
        m_running = false;
    }
    m_executor.stop();
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
    std::string_view s_custom_name, FirmwareType s_firmware_type,
    std::uint32_t s_timeout_ms) noexcept {
    firmware_type_t s_wire_type{};
    if (s_timeout_ms == 0 || s_timeout_ms >= s_kMaximumRelativeTimeout ||
        s_custom_name.size() > s_kMaximumDeviceNameBytes ||
        !s_firmwareTypeToWire(s_firmware_type, s_wire_type)) {
        return {FciEndpointStatus::kInvalidArgument, 0};
    }
    OperationRequest s_request{};
    if (!s_custom_name.empty()) {
        std::memcpy(s_request.m_custom_name.data(), s_custom_name.data(),
                    s_custom_name.size());
    }
    s_request.m_custom_name_size =
        static_cast<std::uint8_t>(s_custom_name.size());
    s_request.m_firmware_type = s_firmware_type;
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
        m_operations[s_index].m_kind != RpcKind::kGetDeviceSettings) {
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

FciEndpointStatus FciWirelinkEndpoint::latestArmStatus(
    FciArmStatusSnapshot& s_status) const noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    if (m_latest_arm_status.m_generation == 0) {
        return FciEndpointStatus::kNoData;
    }
    s_status = m_latest_arm_status;
    return FciEndpointStatus::kOk;
}

FciEndpointStatus FciWirelinkEndpoint::latestDiagnostics(
    ArmDiagnostics& s_diagnostics) const noexcept {
    std::lock_guard<std::mutex> s_lock(m_mutex);
    if (m_diagnostics_generation == 0) return FciEndpointStatus::kNoData;
    s_diagnostics = m_latest_diagnostics;
    return FciEndpointStatus::kOk;
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

void FciWirelinkEndpoint::s_onEvent(void* s_user_data, wl_ctx_t& s_context,
                                    const wl_event_t& s_event) noexcept {
    auto& s_self = *static_cast<FciWirelinkEndpoint*>(s_user_data);
    s_self.m_stats.m_dispatch_calls.fetch_add(1, std::memory_order_relaxed);
    const auto s_result = fci_arm_runtime_dispatch_event(
        &s_context, &s_event, &s_self.m_runtime_instance.runtime, s_nowMs());
    if (s_result.domain != FCI_ARM_RUNTIME_OK &&
        s_result.domain != FCI_ARM_RUNTIME_NON_RX) {
        s_self.m_stats.m_dispatch_errors.fetch_add(1,
                                                  std::memory_order_relaxed);
    }
}

bool FciWirelinkEndpoint::s_applicationProgress(
    void* s_user_data, wl_ctx_t& s_context,
    wl_time_ms_t s_now_ms) noexcept {
    return static_cast<FciWirelinkEndpoint*>(s_user_data)
        ->s_progress(s_context, s_now_ms);
}

std::uint32_t FciWirelinkEndpoint::s_applicationDeadline(
    const void* s_user_data, wl_time_ms_t s_now_ms) noexcept {
    const auto& s_self = *static_cast<const FciWirelinkEndpoint*>(s_user_data);
    wl_rpc_deadline_hint_t s_runtime_hint{WL_RPC_NO_DEADLINE_MS};
    std::uint32_t s_nearest = WL_RPC_NO_DEADLINE_MS;
    if (fci_arm_runtime_get_deadline_hint(&s_self.m_runtime_instance.runtime,
                                          s_now_ms,
                                          &s_runtime_hint) == WL_RPC_OK) {
        s_nearest = s_runtime_hint.next_deadline_ms;
    }

    std::lock_guard<std::mutex> s_lock(s_self.m_mutex);
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

void FciWirelinkEndpoint::s_quiesce(void* s_user_data) noexcept {
    static_cast<FciWirelinkEndpoint*>(s_user_data)->s_stopOnOwner();
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

FciOperationState FciWirelinkEndpoint::s_operationState(
    wl_rpc_client_state_t s_state) noexcept {
    switch (s_state) {
        case WL_RPC_CLIENT_QUEUED: return FciOperationState::kQueued;
        case WL_RPC_CLIENT_LINK_PENDING:
            return FciOperationState::kLinkPending;
        case WL_RPC_CLIENT_WAIT_RESPONSE:
            return FciOperationState::kWaitingResponse;
        case WL_RPC_CLIENT_COMPLETED: return FciOperationState::kCompleted;
        case WL_RPC_CLIENT_LINK_FAILED:
            return FciOperationState::kLinkFailed;
        case WL_RPC_CLIENT_TIMED_OUT: return FciOperationState::kTimedOut;
        case WL_RPC_CLIENT_CANCELLED: return FciOperationState::kCancelled;
        case WL_RPC_CLIENT_APPLICATION_ERROR:
            return FciOperationState::kDomainError;
        default: return FciOperationState::kUnknown;
    }
}

FciEndpointStatus FciWirelinkEndpoint::s_endpointStatus(int s_result) noexcept {
    switch (s_result) {
        case WL_OK: return FciEndpointStatus::kOk;
        case WL_ERR_QUEUE_FULL:
        case WL_ERR_NO_SPACE: return FciEndpointStatus::kQueueFull;
        case WL_ERR_BUSY:
        case WL_ERR_WOULD_BLOCK: return FciEndpointStatus::kBusy;
        case WL_ERR_INVALID_ARG: return FciEndpointStatus::kInvalidArgument;
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
    bool s_progressed = false;
    fci_arm_runtime_poll_result_t s_poll{};
    m_stats.m_runtime_poll_calls.fetch_add(1, std::memory_order_relaxed);
    if (fci_arm_runtime_poll(&m_runtime_instance.runtime, s_now_ms, &s_poll) ==
        WL_RPC_OK) {
        if (s_poll.client_timed_out != 0) {
            m_stats.m_runtime_timeouts.fetch_add(s_poll.client_timed_out,
                                                std::memory_order_relaxed);
            s_progressed = true;
        }
    }

    s_progressed = s_drainLatest() || s_progressed;
    s_progressed = s_finishActive(s_context, s_now_ms) || s_progressed;
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if ((m_lease.m_state == FciControlLeaseState::kRenewQueued ||
             m_lease.m_state == FciControlLeaseState::kRenewing) &&
            s_until(s_now_ms, m_lease_expire_at_ms) == 0) {
            m_lease.m_state = FciControlLeaseState::kExpired;
            m_lease.m_status = FciEndpointStatus::kTimeout;
            m_lease.m_token = 0;
            m_lease.m_granted_timeout_ms = 0;
            s_progressed = true;
        }
    }
    s_progressed = s_scheduleRenewal(s_now_ms) || s_progressed;
    s_progressed = s_startNext(s_context, s_now_ms) || s_progressed;
    return s_progressed;
}

bool FciWirelinkEndpoint::s_drainLatest() noexcept {
    bool s_progressed = false;
    fci_arm_arm_status_latest_view_t s_arm_view{};
    const int s_arm_result = fci_arm_arm_status_latest_acquire(
        &m_runtime_instance.runtime, &s_arm_view);
    if (s_arm_result == WL_OK) {
        m_stats.m_latest_acquires.fetch_add(1, std::memory_order_relaxed);
        if (s_arm_view.value != nullptr && s_validArmStatus(*s_arm_view.value)) {
            const auto s_snapshot =
                s_armStatusFromWire(*s_arm_view.value, s_arm_view.generation);
            ArmStatusCallback s_callback{};
            void* s_user_data{};
            {
                std::lock_guard<std::mutex> s_lock(m_mutex);
                m_latest_arm_status = s_snapshot;
                s_callback = m_arm_status_callback;
                s_user_data = m_callback_user_data;
            }
            if (s_callback != nullptr) s_callback(s_user_data, s_snapshot);
        }
        if (fci_arm_arm_status_latest_release(&m_runtime_instance.runtime,
                                              &s_arm_view) == WL_OK) {
            m_stats.m_latest_releases.fetch_add(1,
                                               std::memory_order_relaxed);
        }
        s_progressed = true;
    }

    fci_arm_arm_diagnostics_latest_view_t s_diagnostics_view{};
    const int s_diagnostics_result = fci_arm_arm_diagnostics_latest_acquire(
        &m_runtime_instance.runtime, &s_diagnostics_view);
    if (s_diagnostics_result == WL_OK) {
        m_stats.m_latest_acquires.fetch_add(1, std::memory_order_relaxed);
        ArmDiagnostics s_domain{};
        if (s_diagnostics_view.value != nullptr &&
            s_diagnosticsFromWire(*s_diagnostics_view.value, s_domain)) {
            DiagnosticsCallback s_callback{};
            void* s_user_data{};
            {
                std::lock_guard<std::mutex> s_lock(m_mutex);
                m_latest_diagnostics = s_domain;
                m_diagnostics_generation = s_diagnostics_view.generation;
                s_callback = m_diagnostics_callback;
                s_user_data = m_callback_user_data;
            }
            if (s_callback != nullptr) s_callback(s_user_data, s_domain);
        }
        if (fci_arm_arm_diagnostics_latest_release(
                &m_runtime_instance.runtime, &s_diagnostics_view) == WL_OK) {
            m_stats.m_latest_releases.fetch_add(1,
                                               std::memory_order_relaxed);
        }
        s_progressed = true;
    }
    return s_progressed;
}

bool FciWirelinkEndpoint::s_finishActive(wl_ctx_t& s_context,
                                         wl_time_ms_t s_now_ms) noexcept {
    std::size_t s_index{};
    RpcKind s_kind{};
    std::uint32_t s_operation_id{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (m_active_operation == s_kNoSlot) return false;
        s_index = m_active_operation;
        s_kind = m_operations[s_index].m_kind;
        s_operation_id = m_operations[s_index].m_operation_id;
    }

    wl_rpc_client_result_t s_client{};
    wl_rpc_err_t s_inspect = WL_RPC_ERR_NOT_FOUND;
    switch (s_kind) {
        case RpcKind::kAcquireLease:
            s_inspect = fci_arm_acquire_control_lease_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kReleaseLease:
            s_inspect = fci_arm_release_control_lease_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kGetDeviceInfo:
            s_inspect = fci_arm_get_device_info_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kSetDeviceInfo:
            s_inspect = fci_arm_set_device_info_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kGetDeviceSettings:
            s_inspect = fci_arm_get_device_settings_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kSetDeviceSettings:
            s_inspect = fci_arm_set_device_settings_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kSetArmControlMode:
            s_inspect = fci_arm_set_arm_control_mode_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kSetGripperControlMode:
            s_inspect = fci_arm_set_gripper_control_mode_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kSetArmMode:
            s_inspect = fci_arm_set_arm_mode_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kHome:
            s_inspect = fci_arm_home_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kSetZero:
            s_inspect = fci_arm_set_zero_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kClearError:
            s_inspect = fci_arm_clear_error_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kClearFaults:
            s_inspect = fci_arm_clear_faults_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kEmergencyStop:
            s_inspect = fci_arm_emergency_stop_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kMotorRegisterRead:
            s_inspect = fci_arm_motor_register_read_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kMotorRegisterWrite:
            s_inspect = fci_arm_motor_register_write_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kMotorStoreParameters:
            s_inspect = fci_arm_motor_store_parameters_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kMotorSetZero:
            s_inspect = fci_arm_motor_set_zero_client_inspect(
                &m_runtime_instance.runtime, s_operation_id, &s_client);
            break;
        case RpcKind::kNone:
            break;
    }
    if (s_inspect != WL_RPC_OK) return false;

    const FciOperationState s_state = s_operationState(s_client.state);
    if (!s_terminal(s_state)) {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        if (m_operations[s_index].m_used) {
            m_operations[s_index].m_state = s_state;
            m_operations[s_index].m_link_status = s_client.link_result;
        }
        return false;
    }

    s_finalize(s_index, s_client, s_context, s_now_ms);
    return true;
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

    fci_arm_runtime_result_t s_started{};
    const fci_arm_encode_scratch_t s_scratch{
        .data = m_rpc_encode_scratch.data(),
        .capacity = m_rpc_encode_scratch.size(),
    };
    switch (s_slot.m_kind) {
        case RpcKind::kAcquireLease: {
            acquire_control_lease_request_t s_request{};
            acquire_control_lease_request_clear(&s_request);
            s_request.has_requested_timeout_ms = true;
            s_request.requested_timeout_ms =
                s_slot.m_request.m_requested_lease_timeout_ms;
            if (s_slot.m_request.m_lease_token != 0) {
                s_request.has_current_token = true;
                s_request.current_token = s_slot.m_request.m_lease_token;
            }
            s_started = fci_arm_acquire_control_lease_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kReleaseLease: {
            release_control_lease_request_t s_request{};
            release_control_lease_request_clear(&s_request);
            s_request.has_lease_token = true;
            s_request.lease_token = s_slot.m_request.m_lease_token;
            s_started = fci_arm_release_control_lease_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kGetDeviceInfo: {
            get_device_info_request_t s_request{};
            get_device_info_request_clear(&s_request);
            s_started = fci_arm_get_device_info_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kSetDeviceInfo: {
            set_device_info_request_t s_request{};
            set_device_info_request_clear(&s_request);
            s_request.has_custom_name = true;
            s_request.custom_name = {
                s_slot.m_request.m_custom_name.data(),
                s_slot.m_request.m_custom_name_size,
            };
            s_request.has_firmware_type = true;
            if (!s_firmwareTypeToWire(s_slot.m_request.m_firmware_type,
                                      s_request.firmware_type)) {
                break;
            }
            s_started = fci_arm_set_device_info_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kGetDeviceSettings: {
            get_device_settings_request_t s_request{};
            get_device_settings_request_clear(&s_request);
            s_started = fci_arm_get_device_settings_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kSetDeviceSettings: {
            set_device_settings_request_t s_request{};
            set_device_settings_request_clear(&s_request);
            s_request.has_settings = true;
            s_settingsToWire(s_slot.m_request.m_device_settings,
                             s_request.settings);
            s_started = fci_arm_set_device_settings_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kSetArmControlMode: {
            set_arm_control_mode_request_t s_request{};
            set_arm_control_mode_request_clear(&s_request);
            s_request.has_mode = true;
            s_request.mode = s_controlMode(s_slot.m_request.m_control_mode);
            s_started = fci_arm_set_arm_control_mode_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kSetGripperControlMode: {
            set_gripper_control_mode_request_t s_request{};
            set_gripper_control_mode_request_clear(&s_request);
            s_request.has_mode = true;
            s_request.mode = s_controlMode(s_slot.m_request.m_control_mode);
            s_started = fci_arm_set_gripper_control_mode_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kSetArmMode: {
            set_arm_mode_request_t s_request{};
            set_arm_mode_request_clear(&s_request);
            s_request.has_mode = true;
            s_request.mode = s_armMode(s_slot.m_request.m_arm_mode);
            s_started = fci_arm_set_arm_mode_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kHome: {
            home_request_t s_request{};
            home_request_clear(&s_request);
            s_started = fci_arm_home_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kSetZero: {
            set_zero_request_t s_request{};
            set_zero_request_clear(&s_request);
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started = fci_arm_set_zero_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kClearError: {
            clear_error_request_t s_request{};
            clear_error_request_clear(&s_request);
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started = fci_arm_clear_error_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kClearFaults: {
            clear_faults_request_t s_request{};
            clear_faults_request_clear(&s_request);
            s_started = fci_arm_clear_faults_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kEmergencyStop: {
            emergency_stop_request_t s_request{};
            emergency_stop_request_clear(&s_request);
            s_started = fci_arm_emergency_stop_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kMotorRegisterRead: {
            motor_register_read_request_t s_request{};
            motor_register_read_request_clear(&s_request);
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_request.has_register_id = true;
            s_request.register_id = s_slot.m_request.m_register_id;
            s_started = fci_arm_motor_register_read_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kMotorRegisterWrite: {
            motor_register_write_request_t s_request{};
            motor_register_write_request_clear(&s_request);
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_request.has_register_id = true;
            s_request.register_id = s_slot.m_request.m_register_id;
            s_request.has_value = true;
            s_request.value = s_slot.m_request.m_value;
            s_started = fci_arm_motor_register_write_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kMotorStoreParameters: {
            motor_store_parameters_request_t s_request{};
            motor_store_parameters_request_clear(&s_request);
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started =
                fci_arm_motor_store_parameters_client_start_scratch(
                    &s_context, &m_runtime_instance.runtime, &s_request,
                    s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kMotorSetZero: {
            motor_set_zero_request_t s_request{};
            motor_set_zero_request_clear(&s_request);
            s_request.has_joint_id = true;
            s_request.joint_id = s_slot.m_request.m_joint_id;
            s_started = fci_arm_motor_set_zero_client_start_scratch(
                &s_context, &m_runtime_instance.runtime, &s_request,
                s_slot.m_timeout_ms, s_now_ms, s_scratch);
            break;
        }
        case RpcKind::kNone:
            break;
    }

    const std::uint32_t s_operation_id =
        s_started.detail_kind == FCI_ARM_RUNTIME_DETAIL_RPC
            ? s_started.detail.rpc.operation_id
            : 0;
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        auto& s_operation = m_operations[s_index];
        s_operation.m_operation_id = s_operation_id;
        if (s_operation_id == 0) {
            s_operation.m_state = FciOperationState::kLinkFailed;
            s_operation.m_status =
                s_started.domain == FCI_ARM_RUNTIME_RPC_ERROR &&
                        s_started.detail.rpc.rpc_result ==
                            WL_RPC_ERR_NO_SLOT
                    ? FciEndpointStatus::kQueueFull
                    : FciEndpointStatus::kLinkError;
            s_operation.m_link_status = s_started.detail.rpc.core_result;
            m_active_operation = s_kNoSlot;
            if (s_operation.m_kind == RpcKind::kAcquireLease) {
                m_lease.m_state =
                    s_operation.m_request.m_lease_token != 0
                                      ? FciControlLeaseState::kExpired
                                      : FciControlLeaseState::kFailed;
                m_lease.m_status = s_operation.m_status;
                m_lease.m_token = 0;
                m_lease.m_granted_timeout_ms = 0;
            } else if (s_operation.m_kind == RpcKind::kReleaseLease) {
                m_lease.m_state = FciControlLeaseState::kFailed;
                m_lease.m_status = s_operation.m_status;
            }
            if (s_operation.m_internal) s_operation = OperationSlot{};
            m_operation_changed.notify_all();
        }
    }
    if (s_operation_id != 0) {
        m_stats.m_rpc_started.fetch_add(1, std::memory_order_relaxed);
    }
    return true;
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
    std::size_t s_active{};
    RpcKind s_kind{};
    std::uint32_t s_operation_id{};
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        s_active = m_active_operation;
        if (s_active != s_kNoSlot) {
            s_kind = m_operations[s_active].m_kind;
            s_operation_id = m_operations[s_active].m_operation_id;
        }
    }

    if (s_active != s_kNoSlot && s_operation_id != 0) {
        wl_rpc_client_result_t s_client{};
        if (wl_rpc_client_get(m_runtime_instance.runtime.rpc_client,
                              s_operation_id, &s_client) == WL_RPC_OK) {
            if (s_client.tx_handle != 0) {
                (void)wl_tx_cancel(&m_executor.context(), s_client.tx_handle);
                wl_tx_result_t s_tx{};
                (void)wl_tx_take(&m_executor.context(), s_client.tx_handle,
                                 &s_tx);
            }
            if (!s_terminal(s_operationState(s_client.state))) {
                (void)wl_rpc_client_cancel(
                    m_runtime_instance.runtime.rpc_client, s_operation_id);
            }
        }
        s_releaseRuntimeOperation(s_kind, s_operation_id);
        m_stats.m_rpc_released.fetch_add(1, std::memory_order_relaxed);
        m_stats.m_rpc_cancelled.fetch_add(1, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> s_lock(m_mutex);
    for (auto& s_slot : m_operations) {
        if (!s_slot.m_used || s_terminal(s_slot.m_state)) continue;
        if (s_slot.m_internal) {
            s_slot = OperationSlot{};
        } else {
            s_slot.m_state = FciOperationState::kCancelled;
            s_slot.m_status = FciEndpointStatus::kCancelled;
        }
    }
    m_active_operation = s_kNoSlot;
    m_lease = FciControlLeaseSnapshot{};
    m_lease.m_status = FciEndpointStatus::kCancelled;
    m_operation_changed.notify_all();
}

void FciWirelinkEndpoint::s_finalize(
    std::size_t s_index, const wl_rpc_client_result_t& s_client,
    wl_ctx_t&, wl_time_ms_t s_now_ms) noexcept {
    RpcKind s_kind{};
    bool s_internal{};
    FixedDeviceInfo s_device_info{};
    FciEndpointStatus s_status{FciEndpointStatus::kInternalError};
    std::int32_t s_domain_status{s_client.application_status};
    const FciOperationState s_state = s_operationState(s_client.state);
    {
        std::lock_guard<std::mutex> s_lock(m_mutex);
        s_kind = m_operations[s_index].m_kind;
        s_internal = m_operations[s_index].m_internal;
    }

    std::uint64_t s_granted_token{};
    std::uint32_t s_granted_timeout{};
    DeviceSettings s_device_settings{};
    float s_motor_register_value{};
    bool s_device_settings_valid{};
    bool s_motor_register_valid{};
    bool s_response_valid = true;
    if (s_state == FciOperationState::kCompleted ||
        s_state == FciOperationState::kDomainError) {
        switch (s_kind) {
            case RpcKind::kAcquireLease: {
                acquire_control_lease_response_t s_response{};
                const auto s_decoded =
                    fci_arm_acquire_control_lease_client_decode(&s_client,
                                                                 &s_response);
                s_response_valid =
                    s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                    s_response.has_status;
                if (s_response_valid) {
                    s_domain_status = s_response.status;
                    if (s_response.status == CONTROL_LEASE_OK) {
                        s_response_valid = s_response.has_lease_token &&
                                           s_response.lease_token != 0 &&
                                           s_response.has_granted_timeout_ms &&
                                           s_response.granted_timeout_ms != 0;
                        s_granted_token = s_response.lease_token;
                        s_granted_timeout = s_response.granted_timeout_ms;
                    }
                }
                break;
            }
            case RpcKind::kReleaseLease: {
                release_control_lease_response_t s_response{};
                const auto s_decoded =
                    fci_arm_release_control_lease_client_decode(&s_client,
                                                                 &s_response);
                s_response_valid =
                    s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                    s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kGetDeviceInfo: {
                get_device_info_response_t s_response{};
                const auto s_decoded = fci_arm_get_device_info_client_decode(
                    &s_client, &s_response);
                s_response_valid =
                    s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                    s_response.has_status;
                if (s_response_valid) {
                    s_domain_status = s_response.status;
                    if (s_response.status == DEVICE_INFO_OK) {
                        const auto& s_info = s_response.info;
                        s_response_valid =
                            s_response.has_info &&
                            s_info.has_protocol_version &&
                            s_info.protocol_version.has_major &&
                            s_info.protocol_version.has_minor &&
                            s_info.protocol_version.has_patch &&
                            s_info.has_firmware_version &&
                            s_info.firmware_version.has_major &&
                            s_info.firmware_version.has_minor &&
                            s_info.firmware_version.has_patch &&
                            s_info.has_board_name && s_info.has_custom_name &&
                            s_info.has_firmware_type;
                        if (s_response_valid) {
                            s_device_info.m_protocol_version = Version{
                                s_info.protocol_version.major,
                                s_info.protocol_version.minor,
                                s_info.protocol_version.patch,
                            };
                            s_device_info.m_firmware_version = Version{
                                s_info.firmware_version.major,
                                s_info.firmware_version.minor,
                                s_info.firmware_version.patch,
                            };
                            s_device_info.m_firmware_type =
                                s_firmwareType(s_info.firmware_type);
                            s_response_valid = s_copyString(
                                s_info.board_name,
                                s_device_info.m_board_name.data(),
                                s_device_info.m_board_name.size(),
                                s_device_info.m_board_name_size);
                            s_response_valid =
                                s_response_valid &&
                                s_copyString(
                                    s_info.custom_name,
                                    s_device_info.m_custom_name.data(),
                                    s_device_info.m_custom_name.size(),
                                    s_device_info.m_custom_name_size);
                            s_device_info.m_valid = s_response_valid;
                        }
                    }
                }
                break;
            }
            case RpcKind::kSetDeviceInfo: {
                set_device_info_response_t s_response{};
                const auto s_decoded = fci_arm_set_device_info_client_decode(
                    &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kGetDeviceSettings: {
                get_device_settings_response_t s_response{};
                const auto s_decoded =
                    fci_arm_get_device_settings_client_decode(&s_client,
                                                               &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) {
                    s_domain_status = s_response.status;
                    if (s_response.status == DEVICE_SETTINGS_OK) {
                        s_response_valid = s_response.has_settings &&
                                           s_settingsFromWire(
                                               s_response.settings,
                                               s_device_settings);
                        s_device_settings_valid = s_response_valid;
                    }
                }
                break;
            }
            case RpcKind::kSetDeviceSettings: {
                set_device_settings_response_t s_response{};
                const auto s_decoded =
                    fci_arm_set_device_settings_client_decode(&s_client,
                                                               &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kSetArmControlMode: {
                set_arm_control_mode_response_t s_response{};
                const auto s_decoded =
                    fci_arm_set_arm_control_mode_client_decode(&s_client,
                                                                &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kSetGripperControlMode: {
                set_gripper_control_mode_response_t s_response{};
                const auto s_decoded =
                    fci_arm_set_gripper_control_mode_client_decode(
                        &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kSetArmMode: {
                set_arm_mode_response_t s_response{};
                const auto s_decoded = fci_arm_set_arm_mode_client_decode(
                    &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kHome: {
                home_response_t s_response{};
                const auto s_decoded =
                    fci_arm_home_client_decode(&s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kSetZero: {
                set_zero_response_t s_response{};
                const auto s_decoded =
                    fci_arm_set_zero_client_decode(&s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kClearError: {
                clear_error_response_t s_response{};
                const auto s_decoded = fci_arm_clear_error_client_decode(
                    &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kClearFaults: {
                clear_faults_response_t s_response{};
                const auto s_decoded = fci_arm_clear_faults_client_decode(
                    &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kEmergencyStop: {
                emergency_stop_response_t s_response{};
                const auto s_decoded = fci_arm_emergency_stop_client_decode(
                    &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kMotorRegisterRead: {
                motor_register_read_response_t s_response{};
                const auto s_decoded =
                    fci_arm_motor_register_read_client_decode(&s_client,
                                                               &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) {
                    s_domain_status = s_response.status;
                    if (s_response.status == MOTOR_OPERATION_OK) {
                        OperationRequest s_request{};
                        {
                            std::lock_guard<std::mutex> s_lock(m_mutex);
                            s_request = m_operations[s_index].m_request;
                        }
                        s_response_valid =
                            s_response.has_joint_id &&
                            s_response.joint_id == s_request.m_joint_id &&
                            s_response.has_register_id &&
                            s_response.register_id ==
                                s_request.m_register_id &&
                            s_response.has_value &&
                            std::isfinite(s_response.value);
                        if (s_response_valid) {
                            s_motor_register_value = s_response.value;
                            s_motor_register_valid = true;
                        }
                    }
                }
                break;
            }
            case RpcKind::kMotorRegisterWrite: {
                motor_register_write_response_t s_response{};
                const auto s_decoded =
                    fci_arm_motor_register_write_client_decode(&s_client,
                                                                &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kMotorStoreParameters: {
                motor_store_parameters_response_t s_response{};
                const auto s_decoded =
                    fci_arm_motor_store_parameters_client_decode(
                        &s_client, &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kMotorSetZero: {
                motor_set_zero_response_t s_response{};
                const auto s_decoded =
                    fci_arm_motor_set_zero_client_decode(&s_client,
                                                          &s_response);
                s_response_valid = s_decoded.domain == FCI_ARM_RUNTIME_OK &&
                                   s_response.has_status;
                if (s_response_valid) s_domain_status = s_response.status;
                break;
            }
            case RpcKind::kNone:
                s_response_valid = false;
                break;
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

    s_releaseRuntimeOperation(s_kind, s_client.operation_id);
    m_stats.m_rpc_released.fetch_add(1, std::memory_order_relaxed);

    std::lock_guard<std::mutex> s_lock(m_mutex);
    auto& s_slot = m_operations[s_index];
    s_slot.m_state = s_state;
    s_slot.m_status = s_status;
    s_slot.m_domain_status = s_domain_status;
    s_slot.m_link_status = s_client.link_result;
    s_slot.m_device_info = s_device_info;
    s_slot.m_device_settings = s_device_settings;
    s_slot.m_motor_register_value = s_motor_register_value;
    s_slot.m_device_settings_valid = s_device_settings_valid;
    s_slot.m_motor_register_valid = s_motor_register_valid;

    if (s_kind == RpcKind::kAcquireLease) {
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

    m_active_operation = s_kNoSlot;
    if (s_internal) s_slot = OperationSlot{};
    m_operation_changed.notify_all();
}

void FciWirelinkEndpoint::s_releaseRuntimeOperation(
    RpcKind s_kind, std::uint32_t s_operation_id) noexcept {
    if (s_operation_id == 0) return;
    switch (s_kind) {
        case RpcKind::kAcquireLease:
            (void)fci_arm_acquire_control_lease_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kReleaseLease:
            (void)fci_arm_release_control_lease_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kGetDeviceInfo:
            (void)fci_arm_get_device_info_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kSetDeviceInfo:
            (void)fci_arm_set_device_info_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kGetDeviceSettings:
            (void)fci_arm_get_device_settings_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kSetDeviceSettings:
            (void)fci_arm_set_device_settings_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kSetArmControlMode:
            (void)fci_arm_set_arm_control_mode_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kSetGripperControlMode:
            (void)fci_arm_set_gripper_control_mode_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kSetArmMode:
            (void)fci_arm_set_arm_mode_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kHome:
            (void)fci_arm_home_client_release(&m_runtime_instance.runtime,
                                              s_operation_id);
            break;
        case RpcKind::kSetZero:
            (void)fci_arm_set_zero_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kClearError:
            (void)fci_arm_clear_error_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kClearFaults:
            (void)fci_arm_clear_faults_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kEmergencyStop:
            (void)fci_arm_emergency_stop_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kMotorRegisterRead:
            (void)fci_arm_motor_register_read_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kMotorRegisterWrite:
            (void)fci_arm_motor_register_write_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kMotorStoreParameters:
            (void)fci_arm_motor_store_parameters_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kMotorSetZero:
            (void)fci_arm_motor_set_zero_client_release(
                &m_runtime_instance.runtime, s_operation_id);
            break;
        case RpcKind::kNone:
            break;
    }
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
            .m_request_id = s_request_id,
            .m_timeout_ms = s_timeout_ms,
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
        .m_operation_id = s_slot.m_operation_id,
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
        .m_runtime_storage_bytes = m_runtime_storage_bytes,
    };
}

} // namespace florid::detail
