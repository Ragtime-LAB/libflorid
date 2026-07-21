#ifndef FLORID_CONTROL_TYPES_HPP
#define FLORID_CONTROL_TYPES_HPP

#include <cstdint>

namespace florid {

enum class ReconnectPolicy {
    kThrow,
    kWait,
};

enum class ControllerMode : std::uint8_t {
    JointImpedance = 0,
    CartesianImpedance = 1,
};

enum class ComputationMode : std::uint8_t {
    kHost = 0,
    kFirmware = 1,
};

struct Finishable {
    bool m_motion_finished{false};
};

struct Torques : Finishable {
    float m_tau[6]{};
    float m_kp[6]{};
    float m_kd[6]{};

    static Torques MotionFinished(const Torques& s_cmd) {
        Torques s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct JointMIT : Finishable {
    float m_q[6]{};
    float m_dq[6]{};
    float m_tau[6]{};
    float m_kp[6]{};
    float m_kd[6]{};

    static JointMIT MotionFinished(const JointMIT& s_cmd) {
        JointMIT s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct JointPosVel : Finishable {
    float m_q[6]{};
    float m_dq[6]{};

    static JointPosVel MotionFinished(const JointPosVel& s_cmd) {
        JointPosVel s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct JointVel : Finishable {
    float m_dq[6]{};

    static JointVel MotionFinished(const JointVel& s_cmd) {
        JointVel s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct JointPVT : Finishable {
    float m_q[6]{};
    float m_dq_limit[6]{};
    float m_current_limit_norm[6]{};

    static JointPVT MotionFinished(const JointPVT& s_cmd) {
        JointPVT s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct CartesianPose : Finishable {
    float m_T[16]{};
    float m_kp[6]{};
    float m_kd[6]{};

    bool hasElbow() const { return false; }

    static CartesianPose MotionFinished(const CartesianPose& s_cmd) {
        CartesianPose s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct CartesianVelocities : Finishable {
    float m_v[6]{};
    float m_kp[6]{};
    float m_kd[6]{};

    static CartesianVelocities MotionFinished(const CartesianVelocities& s_cmd) {
        CartesianVelocities s_r = s_cmd;
        s_r.m_motion_finished = true;
        return s_r;
    }
};

struct TorqueControlDiagnostics {
    double m_actual_hz{};
    std::uint64_t m_period_us_avg{};
    std::uint64_t m_period_us_max{};
    std::uint64_t m_overrun_count{};
    std::uint64_t m_command_age_us{};
    std::uint64_t m_sent_count{};
    std::uint64_t m_last_sdk_timestamp_us{};
    std::uint32_t m_last_sdk_seq{};
    std::uint64_t m_state_age_us{};
    std::uint64_t m_stale_command_count{};
};

} // namespace florid

#endif // FLORID_CONTROL_TYPES_HPP
