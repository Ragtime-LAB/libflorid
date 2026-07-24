#ifndef FLORID_ARM_STATE_HPP
#define FLORID_ARM_STATE_HPP

#include <cstdint>

namespace florid {

struct ArmState {
    double m_time{};
    std::uint32_t m_seq{};
    std::uint32_t m_mode{};
    std::uint64_t m_source_timestamp_us{};
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
