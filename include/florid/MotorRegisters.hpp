#ifndef FLORID_MOTOR_REGISTERS_HPP
#define FLORID_MOTOR_REGISTERS_HPP

#include "florid/Arm.hpp"

#include <cstdint>
#include <cstring>
#include <optional>
#include <tuple>

namespace florid {
namespace motor {

// All functions accept joint_id in range [1, 7] (1–6 = arm, 7 = gripper).

// ── Helpers ────────────────────────────────────────────

inline std::optional<uint32_t> s_readU32(Arm& s_arm, std::uint8_t s_joint, MotorRegister s_rid) {
    auto s_val = s_arm.readMotorRegister(s_joint, s_rid);
    if (!s_val) return std::nullopt;
    uint32_t s_u32 = 0;
    std::memcpy(&s_u32, &*s_val, sizeof(uint32_t));
    return s_u32;
}

// ────────────────────────────────────────────────────────
//  A. Control Loop Gains
// ────────────────────────────────────────────────────────

inline bool setSpeedLoopGains(Arm& s_arm, std::uint8_t s_joint, float s_kp, float s_ki) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::SpeedLoopKp, s_kp)
        && s_arm.writeMotorRegister(s_joint, MotorRegister::SpeedLoopKi, s_ki);
}

inline std::tuple<std::optional<float>, std::optional<float>>
getSpeedLoopGains(Arm& s_arm, std::uint8_t s_joint) {
    return {s_arm.readMotorRegister(s_joint, MotorRegister::SpeedLoopKp),
            s_arm.readMotorRegister(s_joint, MotorRegister::SpeedLoopKi)};
}

inline bool setPositionLoopGains(Arm& s_arm, std::uint8_t s_joint, float s_kp, float s_ki) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::PositionLoopKp, s_kp)
        && s_arm.writeMotorRegister(s_joint, MotorRegister::PositionLoopKi, s_ki);
}

inline std::tuple<std::optional<float>, std::optional<float>>
getPositionLoopGains(Arm& s_arm, std::uint8_t s_joint) {
    return {s_arm.readMotorRegister(s_joint, MotorRegister::PositionLoopKp),
            s_arm.readMotorRegister(s_joint, MotorRegister::PositionLoopKi)};
}

inline bool setSpeedLoopFilter(Arm& s_arm, std::uint8_t s_joint, float s_bw) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::SpeedLoopFilterBW, s_bw);
}

inline std::optional<float> getSpeedLoopFilter(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::SpeedLoopFilterBW);
}

inline bool setSpeedLoopDamping(Arm& s_arm, std::uint8_t s_joint, float s_deta) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::SpeedLoopDamping, s_deta);
}

inline std::optional<float> getSpeedLoopDamping(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::SpeedLoopDamping);
}

inline bool setCurrentLoopBandwidth(Arm& s_arm, std::uint8_t s_joint, float s_bw) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::CurrentLoopBandwidth, s_bw);
}

inline std::optional<float> getCurrentLoopBandwidth(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::CurrentLoopBandwidth);
}

inline bool setCurrentEnhanceFactor(Arm& s_arm, std::uint8_t s_joint, float s_c1) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::CurrentEnhanceFactor, s_c1);
}

inline std::optional<float> getCurrentEnhanceFactor(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::CurrentEnhanceFactor);
}

inline bool setVelocityEnhanceFactor(Arm& s_arm, std::uint8_t s_joint, float s_c1) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::VelocityEnhanceFactor, s_c1);
}

inline std::optional<float> getVelocityEnhanceFactor(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::VelocityEnhanceFactor);
}

// ────────────────────────────────────────────────────────
//  B. Motion Protection
// ────────────────────────────────────────────────────────

inline bool setVoltageLimits(Arm& s_arm, std::uint8_t s_joint, float s_under, float s_over) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::VoltageUnder, s_under)
        && s_arm.writeMotorRegister(s_joint, MotorRegister::VoltageOver, s_over);
}

inline std::tuple<std::optional<float>, std::optional<float>>
getVoltageLimits(Arm& s_arm, std::uint8_t s_joint) {
    return {s_arm.readMotorRegister(s_joint, MotorRegister::VoltageUnder),
            s_arm.readMotorRegister(s_joint, MotorRegister::VoltageOver)};
}

inline bool setTemperatureLimit(Arm& s_arm, std::uint8_t s_joint, float s_ot) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::TemperatureLimit, s_ot);
}

inline std::optional<float> getTemperatureLimit(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::TemperatureLimit);
}

inline bool setOvercurrentLimit(Arm& s_arm, std::uint8_t s_joint, float s_oc) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::OvercurrentLimit, s_oc);
}

inline std::optional<float> getOvercurrentLimit(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::OvercurrentLimit);
}

inline bool setAcceleration(Arm& s_arm, std::uint8_t s_joint, float s_acc) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::Acceleration, s_acc);
}

inline std::optional<float> getAcceleration(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::Acceleration);
}

inline bool setDeceleration(Arm& s_arm, std::uint8_t s_joint, float s_dec) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::Deceleration, s_dec);
}

inline std::optional<float> getDeceleration(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::Deceleration);
}

inline bool setMaxSpeed(Arm& s_arm, std::uint8_t s_joint, float s_spd) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::MaxSpeed, s_spd);
}

inline std::optional<float> getMaxSpeed(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::MaxSpeed);
}

// ────────────────────────────────────────────────────────
//  C. Mapping Ranges
// ────────────────────────────────────────────────────────

inline bool setMappingRanges(Arm& s_arm, std::uint8_t s_joint,
                              float s_pmax, float s_vmax, float s_tmax) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::PositionMax, s_pmax)
        && s_arm.writeMotorRegister(s_joint, MotorRegister::VelocityMax, s_vmax)
        && s_arm.writeMotorRegister(s_joint, MotorRegister::TorqueMax, s_tmax);
}

inline std::tuple<std::optional<float>, std::optional<float>, std::optional<float>>
getMappingRanges(Arm& s_arm, std::uint8_t s_joint) {
    return {s_arm.readMotorRegister(s_joint, MotorRegister::PositionMax),
            s_arm.readMotorRegister(s_joint, MotorRegister::VelocityMax),
            s_arm.readMotorRegister(s_joint, MotorRegister::TorqueMax)};
}

// ────────────────────────────────────────────────────────
//  D. Motor Constants
// ────────────────────────────────────────────────────────

inline bool setTorqueConstant(Arm& s_arm, std::uint8_t s_joint, float s_kt) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::TorqueConstant, s_kt);
}

inline std::optional<float> getTorqueConstant(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::TorqueConstant);
}

inline bool setGearEfficiency(Arm& s_arm, std::uint8_t s_joint, float s_gref) {
    return s_arm.writeMotorRegister(s_joint, MotorRegister::GearEfficiency, s_gref);
}

inline std::optional<float> getGearEfficiency(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::GearEfficiency);
}

// ────────────────────────────────────────────────────────
//  E. Read-Only Diagnostics
// ────────────────────────────────────────────────────────

inline std::optional<float> getDampingCoefficient(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::DampingCoefficient);
}

inline std::optional<float> getInertia(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::Inertia);
}

inline std::optional<uint32_t> getHardwareVersion(Arm& s_arm, std::uint8_t s_joint) {
    return s_readU32(s_arm, s_joint, MotorRegister::HardwareVersion);
}

inline std::optional<uint32_t> getSoftwareVersion(Arm& s_arm, std::uint8_t s_joint) {
    return s_readU32(s_arm, s_joint, MotorRegister::SoftwareVersion);
}

inline std::optional<uint32_t> getPolePairs(Arm& s_arm, std::uint8_t s_joint) {
    return s_readU32(s_arm, s_joint, MotorRegister::PolePairs);
}

inline std::optional<float> getPhaseResistance(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::PhaseResistance);
}

inline std::optional<float> getPhaseInductance(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::PhaseInductance);
}

inline std::optional<float> getFluxLinkage(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::FluxLinkage);
}

inline std::optional<float> getGearRatio(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::GearRatio);
}

inline std::optional<uint32_t> getSubVersion(Arm& s_arm, std::uint8_t s_joint) {
    return s_readU32(s_arm, s_joint, MotorRegister::SubVersion);
}

inline std::optional<float> getMotorPosition(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::MotorPosition);
}

inline std::optional<float> getOutputPosition(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.readMotorRegister(s_joint, MotorRegister::OutputPosition);
}

// ────────────────────────────────────────────────────────
//  F. Special Operations
// ────────────────────────────────────────────────────────

inline bool setZeroPoint(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.setZeroPoint(s_joint);
}

inline bool storeParameters(Arm& s_arm, std::uint8_t s_joint) {
    return s_arm.storeParameters(s_joint);
}

} // namespace motor
} // namespace florid

#endif // FLORID_MOTOR_REGISTERS_HPP
