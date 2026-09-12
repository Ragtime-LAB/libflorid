#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace florid {

enum class UnderwaterMode : std::int32_t {
    kDamp = 0, kGravity = 1, kTeleop = 2, kControl = 3, kUsb = kControl, kZeroing = 4, kRetract = 5,
};

enum class UnderwaterBusinessStatus : std::int32_t {
    kOk = 0, kInvalidMode = 1, kInvalidState = 2, kBusy = 3,
    kRejected = 4, kInternalError = 5,
};

enum class UnderwaterRpcError {
    kNone, kNotConnected, kInvalidArgument, kBusy, kTimeout,
    kTransportError, kProtocolError, kCancelled, kDeviceRejected,
};

// Motor order: master j1..j4, master gripper, slave j1..j4, slave gripper.
// All command/state coordinates are motor rad, rad/s, Nm.
enum class UnderwaterControlKind : std::uint8_t { kMIT = 1, kMitWithGravity = 2 };
struct UnderwaterArmCommand {
    std::array<float, 10> m_q{}, m_dq{}, m_kp{}, m_kd{}, m_tau{};
    UnderwaterControlKind m_kind{UnderwaterControlKind::kMIT};
};
struct UnderwaterArmStatus {
    std::array<float, 10> m_q{}, m_dq{}, m_tau{};
    std::array<std::uint8_t, 10> m_temperature_c{}, m_motor_status{};
    std::array<std::uint32_t, 10> m_feedback_age_ms{};
    std::uint16_t m_fresh_mask{}, m_enabled_mask{}, m_available_mask{}, m_applied_mask{};
    UnderwaterMode m_mode{UnderwaterMode::kDamp};
    UnderwaterControlKind m_kind{UnderwaterControlKind::kMitWithGravity};
    std::uint32_t m_sequence{}, m_applied_command_sequence{};
    std::uint64_t m_timestamp_us{}, m_control_session{}, m_last_sdk_timestamp_us{};
    bool m_command_received{};
    std::uint8_t m_control_stop_reason{}, m_command_reject_reason{};
    std::uint32_t m_commands_received{}, m_commands_overwritten{}, m_commands_rejected{};
};
struct UnderwaterCommandResult {
    UnderwaterRpcError m_error{UnderwaterRpcError::kNone};
    std::uint32_t m_sequence{};
    // Local send admission; ArmStatus applied_command_sequence/applied_mask
    // report which command the control thread has submitted to CAN.
    explicit operator bool() const noexcept { return m_error == UnderwaterRpcError::kNone; }
};

struct UnderwaterStatus {
    UnderwaterMode m_mode{UnderwaterMode::kDamp};
    std::uint32_t m_uptime_ms{};
    std::uint64_t m_control_session{};
    // Arm bits: bit 0 master, bit 1 slave. Motor bits: bit 0..3 joints,
    // bit 4 gripper. Availability and enabled masks are observed firmware state.
    std::uint32_t m_enabling_arms{}, m_gravity_arms{};
    std::uint32_t m_master_available_mask{}, m_slave_available_mask{};
    std::uint32_t m_master_enabled_mask{}, m_slave_enabled_mask{};
    std::uint32_t m_master_disable_pending_mask{}, m_slave_disable_pending_mask{};
    // Present only on firmware with explicit fresh disabled feedback reporting.
    std::optional<std::uint32_t> m_master_disabled_mask, m_slave_disabled_mask;
    bool m_upgrading{};
    std::string m_reason;
};

struct UnderwaterRpcResult {
    UnderwaterRpcError m_error{UnderwaterRpcError::kNone};
    std::int32_t m_device_status{};
    std::optional<UnderwaterStatus> m_info;
    // True means the firmware processed/accepted this request. Enabling and
    // Retract may still be in progress: inspect the snapshot, then query again.
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_error == UnderwaterRpcError::kNone && m_device_status == 0 &&
               m_info.has_value();
    }
};

enum class UnderwaterSettingsBusinessStatus : std::int32_t {
    kOk = 0, kBusy = 1, kStorageFailed = 2, kInternalError = 3,
};

enum class UnderwaterSettingsSource : std::int32_t {
    kDefaults = 0, kStored = 1, kCorrupt = 2, kStorageError = 3,
};

struct UnderwaterSettings {
    // Five-axis arrays: joints 1..4, then gripper, in motor/teleop ordering.
    std::array<float, 5> m_angle_scale{}, m_angle_offset_rad{}, m_kp{}, m_kd{};
    // Master joints 1..4, then slave joints 1..4.
    std::array<float, 8> m_gravity_scale{};
    // Master joints 1..4 + gripper, then slave joints 1..4 + gripper.
    std::array<float, 10> m_friction_coulomb_positive_nm{}, m_friction_coulomb_negative_nm{};
    std::array<float, 10> m_friction_viscous_nm_per_rad_s{}, m_friction_deadband_rad_s{};
    std::array<float, 10> m_friction_transition_rad_s{}, m_friction_filter_time_constant_s{};
    std::array<float, 5> m_force_gain{}, m_force_low_gain{};
    float m_force_low_load_nm{}, m_force_high_load_nm{};
    std::array<float, 5> m_force_velocity_filter_s{}, m_force_load_filter_s{};
    bool m_buzzer_alarm_enabled{true};
};

struct UnderwaterSettingsSnapshot {
    UnderwaterSettings m_settings;
    std::uint32_t m_revision{};
    UnderwaterSettingsSource m_source{UnderwaterSettingsSource::kDefaults};
    std::int32_t m_storage_error{};
};

struct UnderwaterSettingsRpcResult {
    UnderwaterRpcError m_error{UnderwaterRpcError::kNone};
    std::int32_t m_device_status{};
    std::optional<UnderwaterSettingsSnapshot> m_snapshot;
    std::string m_reason;
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_error == UnderwaterRpcError::kNone && m_device_status == 0 &&
               m_snapshot.has_value();
    }
};

} // namespace florid
