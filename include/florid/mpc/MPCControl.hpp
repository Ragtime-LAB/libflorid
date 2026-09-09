#pragma once

#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

namespace florid {
namespace detail { class MPCSession; }

// Willow position MPC. Planning and interpolation run inside the SDK.
struct MPCControlConfig {
    static constexpr auto planning_period = std::chrono::milliseconds(20);
    static constexpr auto output_period = std::chrono::microseconds(2000);
    float current_limit_norm = 0.3f;
    float velocity_excitation = 1.2f;
    float max_joint_velocity = 1.0f;       // rad/s, also constrains the OCP
    float max_joint_acceleration = 20.0f;  // rad/s^2, checked over each spline
    float max_tracking_error = 0.15f;     // rad, measured vs executing reference
    std::chrono::milliseconds state_timeout{40};
    std::chrono::milliseconds target_timeout{200};
    std::chrono::milliseconds plan_timeout{60};
    std::chrono::milliseconds output_lateness_limit{10};
};

enum class MPCControlState { kWaitingForTarget, kPlanning, kRunning, kStopped, kFault };
enum class MPCStopReason {
    kNone, kStopped, kMotionFinished, kStaleState, kStaleTarget, kStalePlan,
    kOutputDeadline, kInvalidState, kInvalidTrajectory, kTrackingError, kTransport, kInternalError
};
const char* mpcStopReasonMessage(MPCStopReason s_reason) noexcept;

// Planner and output fields are independently coherent latest snapshots, not a
// transaction across workers. After stop() returns, both contain final counters.
struct MPCControlStatus {
    MPCControlState m_state{MPCControlState::kWaitingForTarget};
    MPCStopReason m_stop_reason{MPCStopReason::kNone};
    int m_solver_status{-1};
    std::uint64_t m_solves{}, m_solver_failures{}, m_outputs{}, m_plan_switches{};
    std::uint64_t m_missed_planning_periods{}, m_missed_output_periods{};
    double m_last_solve_ms{}, m_max_solve_ms{}, m_max_output_lateness_ms{};
    double m_state_age_ms{}, m_plan_age_ms{};
    float m_reference_q[6]{}, m_reference_dq[6]{};
};

// A session owns its workers. Destruction/stop/MotionFinished stops output and
// joins them; a stopped handle cannot resume, even after another session starts.
class CartesianMPCControl {
public:
    ~CartesianMPCControl();
    CartesianMPCControl(const CartesianMPCControl&) = delete;
    CartesianMPCControl& operator=(const CartesianMPCControl&) = delete;
    ArmState readOnce(); // latest snapshot; does not consume Arm::readOnce()
    void writeOnce(const CartesianPose& s_target);
    MPCControlStatus status() const;
    void stop() noexcept;

private:
    friend class Arm;
    CartesianMPCControl(std::shared_ptr<void> s_owner,
                        std::shared_ptr<detail::MPCSession> s_session,
                        std::function<ArmState()> s_read);
    std::shared_ptr<void> m_owner;
    std::shared_ptr<detail::MPCSession> m_session;
    std::function<ArmState()> m_read;
};
} // namespace florid
