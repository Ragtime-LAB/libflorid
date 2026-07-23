#include "florid/core/GripperCore.hpp"

namespace florid {

fci::arm::GripperCommandPacket GripperCore::s_pack(const JointMIT& s_cmd) {
    fci::arm::GripperCommandPacket s_pkt{};
    s_pkt.q = s_cmd.m_q[0];
    s_pkt.dq = s_cmd.m_dq[0];
    s_pkt.tau = s_cmd.m_tau[0];
    s_pkt.kp = s_cmd.m_kp[0];
    s_pkt.kd = s_cmd.m_kd[0];
    s_pkt.control_mode = 1;
    if (s_cmd.m_firmware_gravity) s_pkt.control_mode |= 0x04;
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::GripperPosVelCommandPacket GripperCore::s_pack(const JointPosVel& s_cmd) {
    fci::arm::GripperPosVelCommandPacket s_pkt{};
    s_pkt.q = s_cmd.m_q[0];
    s_pkt.dq = s_cmd.m_dq[0];
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::GripperVelCommandPacket GripperCore::s_pack(const JointVel& s_cmd) {
    fci::arm::GripperVelCommandPacket s_pkt{};
    s_pkt.dq = s_cmd.m_dq[0];
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::GripperPVTCommandPacket GripperCore::s_pack(const JointPVT& s_cmd) {
    fci::arm::GripperPVTCommandPacket s_pkt{};
    s_pkt.q = s_cmd.m_q[0];
    s_pkt.dq_limit = s_cmd.m_dq_limit[0];
    s_pkt.current_limit_norm = s_cmd.m_current_limit_norm[0];
    s_pkt.seq = nextSeq();
    return s_pkt;
}

} // namespace florid
