#ifndef FLORID_ARM_STATE_HPP
#define FLORID_ARM_STATE_HPP

#include <cstdint>

namespace florid {

enum class ArmMode : std::uint8_t {
    Init = 0,
    Idle = 1,
    Running = 2,
    Fault = 3,
    EStop = 4,
};

struct ArmState {
    double m_time{};
    std::uint32_t m_seq{};
    std::uint64_t m_source_timestamp_us{};
    ArmMode m_mode{};
    std::uint32_t m_errors{};
    float m_q[6]{};
    float m_dq[6]{};
    float m_tau[6]{};
    float m_base_gravity[3]{};
    float m_O_T_EE[16]{};
    float m_F_ext[6]{};
    float m_gripper_q{};
    float m_gripper_dq{};
    float m_gripper_tau{};
};

} // namespace florid

#endif // FLORID_ARM_STATE_HPP
