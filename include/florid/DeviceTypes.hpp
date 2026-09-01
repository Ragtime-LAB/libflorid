#ifndef FLORID_DEVICE_TYPES_HPP
#define FLORID_DEVICE_TYPES_HPP

#include <array>
#include <cstdint>
#include <string>

namespace florid {

struct Version {
    std::uint32_t m_major{};
    std::uint32_t m_minor{};
    std::uint32_t m_patch{};
};

enum class FirmwareType : std::uint8_t {
    kStandardArm = 0,
    kMobileArm = 1,
    kCobotArm = 2,
    kUnknown = 0xff,
};

enum class BusState : std::uint8_t {
    kErrorActive = 0,
    kErrorWarning = 1,
    kErrorPassive = 2,
    kBusOff = 3,
    kStopped = 4,
    kUnknown = 0xff,
};

enum class MotorRegister : std::uint8_t {
    TorqueConstant = 0x01,
    GearEfficiency = 0x1e,
    CurrentLoopBandwidth = 0x18,
    SpeedLoopKp = 0x19,
    SpeedLoopKi = 0x1a,
    PositionLoopKp = 0x1b,
    PositionLoopKi = 0x1c,
    SpeedLoopDamping = 0x1f,
    SpeedLoopFilterBW = 0x20,
    CurrentEnhanceFactor = 0x21,
    VelocityEnhanceFactor = 0x22,
    VoltageUnder = 0x00,
    VoltageOver = 0x1d,
    TemperatureLimit = 0x02,
    OvercurrentLimit = 0x03,
    Acceleration = 0x04,
    Deceleration = 0x05,
    MaxSpeed = 0x06,
    PositionMax = 0x15,
    VelocityMax = 0x16,
    TorqueMax = 0x17,
    DampingCoefficient = 0x0b,
    Inertia = 0x0c,
    HardwareVersion = 0x0d,
    SoftwareVersion = 0x0e,
    PolePairs = 0x10,
    PhaseResistance = 0x11,
    PhaseInductance = 0x12,
    FluxLinkage = 0x13,
    GearRatio = 0x14,
    SubVersion = 0x24,
    MotorPosition = 0x50,
    OutputPosition = 0x51,
};

struct DeviceInfo {
    Version m_protocol_version{};
    Version m_firmware_version{};
    std::string m_board_name;
    std::string m_custom_name;
    FirmwareType m_firmware_type{FirmwareType::kUnknown};
};

struct TorqueFoldParameters {
    float m_continuous_torque{};
    float m_peak_torque{};
    float m_thermal_capacity{};
    float m_torque_ramp_rate{};
};

struct JointLimits {
    float m_min{};
    float m_max{};
};

struct DeviceSettings {
    std::uint32_t m_firmware_period_us{2000};
    std::array<float, 6> m_gravity_scale{};
    std::array<TorqueFoldParameters, 7> m_torque_fold{};
    std::array<JointLimits, 6> m_joint_limits{};
};

struct JointDiagnostics {
    bool m_healthy{};
    float m_temperature_c{};
};

struct GripperDiagnostics {
    bool m_healthy{};
    float m_temperature_c{};
};

struct ArmDiagnostics {
    std::uint32_t m_uptime_s{};
    std::uint32_t m_tick_count{};
    std::uint32_t m_mode_entry_ms{};
    bool m_bus_healthy{};
    BusState m_bus_state{BusState::kUnknown};
    std::uint16_t m_tx_error_count{};
    std::uint16_t m_rx_error_count{};
    std::array<JointDiagnostics, 6> m_joints{};
    GripperDiagnostics m_gripper{};
};

} // namespace florid

#endif // FLORID_DEVICE_TYPES_HPP
