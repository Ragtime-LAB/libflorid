#ifndef FLORID_GRIPPER_STATE_HPP
#define FLORID_GRIPPER_STATE_HPP

#include <cstdint>

namespace florid {

struct GripperState {
    float m_q{};
    float m_dq{};
    float m_tau{};
};

} // namespace florid

#endif // FLORID_GRIPPER_STATE_HPP
