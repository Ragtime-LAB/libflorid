#include "florid/core/ArmCore.hpp"

namespace florid {

static void s_copyFloats(const float* s_src, float* s_dst, std::uint8_t s_n) {
    for (std::uint8_t s_i = 0; s_i < s_n; ++s_i) s_dst[s_i] = s_src[s_i];
}

fci::arm::JointMITCommandPacket ArmCore::s_pack(const Torques& s_cmd) {
    fci::arm::JointMITCommandPacket s_pkt{};
    for (int s_i = 0; s_i < 6; ++s_i) {
        s_pkt.tau[s_i] = s_cmd.m_tau[s_i];
        s_pkt.kp[s_i] = s_cmd.m_kp[s_i];
        s_pkt.kd[s_i] = s_cmd.m_kd[s_i];
        s_pkt.q[s_i] = 0.0f;
        s_pkt.dq[s_i] = 0.0f;
    }
    s_pkt.control_mode = 3; // torque mode
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::JointMITCommandPacket ArmCore::s_pack(const JointMIT& s_cmd) {
    fci::arm::JointMITCommandPacket s_pkt{};
    s_copyFloats(s_cmd.m_q, s_pkt.q, 6);
    s_copyFloats(s_cmd.m_dq, s_pkt.dq, 6);
    s_copyFloats(s_cmd.m_tau, s_pkt.tau, 6);
    s_copyFloats(s_cmd.m_kp, s_pkt.kp, 6);
    s_copyFloats(s_cmd.m_kd, s_pkt.kd, 6);
    s_pkt.control_mode = 1; // MIT mode
    if (s_cmd.m_firmware_gravity) s_pkt.control_mode |= 0x04; // bind bit2 = gravity_enable
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::JointPosVelCommandPacket ArmCore::s_pack(const JointPosVel& s_cmd) {
    fci::arm::JointPosVelCommandPacket s_pkt{};
    s_copyFloats(s_cmd.m_q, s_pkt.q, 6);
    s_copyFloats(s_cmd.m_dq, s_pkt.dq, 6);
    s_pkt.enabled_mask = 0x3F; // all 6 joints enabled
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::JointVelCommandPacket ArmCore::s_pack(const JointVel& s_cmd) {
    fci::arm::JointVelCommandPacket s_pkt{};
    s_copyFloats(s_cmd.m_dq, s_pkt.dq, 6);
    s_pkt.enabled_mask = 0x3F;
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::JointPVTCommandPacket ArmCore::s_pack(const JointPVT& s_cmd) {
    fci::arm::JointPVTCommandPacket s_pkt{};
    s_copyFloats(s_cmd.m_q, s_pkt.q, 6);
    s_copyFloats(s_cmd.m_dq_limit, s_pkt.dq_limit, 6);
    s_copyFloats(s_cmd.m_current_limit_norm, s_pkt.current_limit_norm, 6);
    s_pkt.enabled_mask = 0x3F;
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::CartesianPoseCommandPacket ArmCore::s_pack(const CartesianPose& s_cmd) {
    fci::arm::CartesianPoseCommandPacket s_pkt{};
    s_copyFloats(s_cmd.m_T, s_pkt.T, 16);
    s_copyFloats(s_cmd.m_kp, s_pkt.kp, 6);
    s_copyFloats(s_cmd.m_kd, s_pkt.kd, 6);
    s_pkt.control_mode = 1; // position mode
    s_pkt.seq = nextSeq();
    return s_pkt;
}

fci::arm::CartesianVelocityCommandPacket ArmCore::s_pack(const CartesianVelocities& s_cmd) {
    fci::arm::CartesianVelocityCommandPacket s_pkt{};
    s_copyFloats(s_cmd.m_v, s_pkt.v, 6);
    s_copyFloats(s_cmd.m_kp, s_pkt.kp, 6);
    s_copyFloats(s_cmd.m_kd, s_pkt.kd, 6);
    s_pkt.control_mode = 2; // velocity mode
    s_pkt.seq = nextSeq();
    return s_pkt;
}

} // namespace florid
