#include "florid/detail/MPCSession.hpp"
#include "florid/Exceptions.hpp"
#include "WillowMPCTraits.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <cstdio>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
#endif

namespace florid {
const char* mpcStopReasonMessage(MPCStopReason s_reason) noexcept {
    switch (s_reason) {
        case MPCStopReason::kNone: return "none";
        case MPCStopReason::kStopped: return "stopped by caller";
        case MPCStopReason::kMotionFinished: return "motion finished";
        case MPCStopReason::kStaleState: return "joint feedback timed out";
        case MPCStopReason::kStaleTarget: return "Cartesian target timed out";
        case MPCStopReason::kStalePlan: return "no fresh valid MPC plan";
        case MPCStopReason::kOutputDeadline: return "output scheduling deadline missed";
        case MPCStopReason::kInvalidState: return "invalid joint feedback or device fault";
        case MPCStopReason::kInvalidTrajectory: return "interpolated trajectory exceeds limits";
        case MPCStopReason::kTrackingError: return "joint tracking error exceeds limit";
        case MPCStopReason::kTransport: return "transport disconnected or command rejected";
        case MPCStopReason::kInternalError: return "MPC mailbox invariant failed";
    }
    return "unknown";
}

CartesianMPCControl::CartesianMPCControl(std::shared_ptr<void> s_owner,
    std::shared_ptr<detail::MPCSession> s_session, std::function<ArmState()> s_read)
    : m_owner(std::move(s_owner)), m_session(std::move(s_session)), m_read(std::move(s_read)) {}
CartesianMPCControl::~CartesianMPCControl() { stop(); }
ArmState CartesianMPCControl::readOnce() { return m_read(); }
void CartesianMPCControl::writeOnce(const CartesianPose& s_target) { m_session->write(s_target); }
MPCControlStatus CartesianMPCControl::status() const { return m_session->status(); }
void CartesianMPCControl::stop() noexcept { m_session->stop(); }
} // namespace florid

namespace florid::detail {
namespace {
double seconds(MPCClock::duration s_duration) {
    return std::chrono::duration<double>(s_duration).count();
}
double millis(MPCClock::duration s_duration) { return seconds(s_duration) * 1000; }

// Diagnostic branch only: dump after a rejection, before stopping output.
void traceCurve(const char* label, const MPCJointState& from, const MPCJointState& to,
                const MPCJointLimits& limits, double dt) {
    std::fprintf(stderr, "REJECTED_CURVE %s dt=%.17g\n", label, dt);
    for (int i = 0; i < 6; ++i) {
        const double delta = to.m_q[i] - from.m_q[i];
        const double a = -2*delta + dt*(from.m_dq[i]+to.m_dq[i]);
        const double b = 3*delta - dt*(2*from.m_dq[i]+to.m_dq[i]);
        const double c = dt*from.m_dq[i], d = from.m_q[i];
        auto q = [&](double u) { return ((a*u+b)*u+c)*u+d; };
        auto v = [&](double u) { return ((3*a*u+2*b)*u+c)/dt; };
        double qmin = std::min(q(0),q(1)), qmax = std::max(q(0),q(1));
        double vmax = std::max(std::abs(v(0)),std::abs(v(1)));
        auto extremum = [&](double u) {
            if (u > 0 && u < 1) { qmin = std::min(qmin,q(u)); qmax = std::max(qmax,q(u)); }
        };
        if (std::abs(a) > 1e-15) {
            double u = -b/(3*a);
            if (u > 0 && u < 1) vmax = std::max(vmax,std::abs(v(u)));
            const double disc = b*b-3*a*c;
            if (disc >= 0) { extremum((-b+std::sqrt(disc))/(3*a)); extremum((-b-std::sqrt(disc))/(3*a)); }
        } else if (std::abs(b) > 1e-15) extremum(-c/(2*b));
        const double amax = std::max(std::abs(2*b),std::abs(6*a+2*b))/(dt*dt);
        std::fprintf(stderr,"CURVE_JOINT %d q0=%.17g dq0=%.17g q1=%.17g dq1=%.17g qmin=%.17g qmax=%.17g vmax=%.17g amax=%.17g limits=[%.17g,%.17g,%.17g,%.17g]\n",
            i,from.m_q[i],from.m_dq[i],to.m_q[i],to.m_dq[i],qmin,qmax,vmax,amax,
            limits.m_lower[i],limits.m_upper[i],limits.m_velocity[i],limits.m_acceleration[i]);
    }
}
void tracePlan(const char* label, const MPCPlan& plan, MPCClock::time_point now) {
    std::fprintf(stderr,"PLAN %s gen=%llu age=%.17g ms\n",label,
        static_cast<unsigned long long>(plan.m_generation),millis(now-plan.m_origin));
    for (std::size_t k=0; k<plan.m_knots.size(); ++k) {
        std::fprintf(stderr,"KNOT %s %zu",label,k);
        for (int i=0;i<6;++i) std::fprintf(stderr," %.17g %.17g",plan.m_knots[k].m_q[i],plan.m_knots[k].m_dq[i]);
        std::fputc('\n',stderr);
    }
}

// Match the timer-resolution request to the output worker's lifetime. This
// improves Windows timed waits; it does not guarantee real-time scheduling.
class TimerResolution {
public:
#ifdef _WIN32
    TimerResolution() : m_ok(timeBeginPeriod(1) == TIMERR_NOERROR) {}
    ~TimerResolution() { if (m_ok) timeEndPeriod(1); }
    bool valid() const { return m_ok; }
private:
    bool m_ok;
#else
    bool valid() const { return true; }
#endif
};
}

void MPCSession::validateConfig(const MPCControlConfig& c) {
    if (!std::isfinite(c.current_limit_norm) || c.current_limit_norm <= 0 || c.current_limit_norm > 1 ||
        !std::isfinite(c.velocity_excitation) || c.velocity_excitation < 1 ||
        !std::isfinite(c.max_joint_velocity) || c.max_joint_velocity <= 0 ||
        !std::isfinite(c.max_joint_acceleration) || c.max_joint_acceleration <= 0 ||
        !std::isfinite(c.max_tracking_error) || c.max_tracking_error <= 0 ||
        c.state_timeout <= c.output_period || c.target_timeout <= c.planning_period ||
        c.plan_timeout <= c.planning_period || c.plan_timeout > std::chrono::milliseconds(80) ||
        c.output_lateness_limit < c.output_period || c.output_lateness_limit >= c.plan_timeout)
        throw CommandException("Invalid MPC control limits or deadlines");
}

MPCSession::MPCSession(MPCControlConfig s_config, Read s_read, Send s_send, Solve s_solve)
    : MPCSession(s_config, s_read, s_read, std::move(s_send), std::move(s_solve)) {}

MPCSession::MPCSession(MPCControlConfig s_config, Read s_plan_read, Read s_output_read,
                       Send s_send, Solve s_solve)
    : m_config(s_config), m_plan_read(std::move(s_plan_read)),
      m_output_read(std::move(s_output_read)), m_send(std::move(s_send)), m_solve(std::move(s_solve)) {
    validateConfig(m_config);
    if (!m_plan_read || !m_output_read || !m_send || !m_solve)
        throw ControlException("Missing MPC session callback");
    if (!m_stop_reason.is_lock_free() || !m_started.is_lock_free() || !m_target_time_ns.is_lock_free())
        throw ControlException("MPC requires lock-free control atomics on this platform");
    for (int i = 0; i < 6; ++i) {
        m_limits.m_lower[i] = WillowMPCTraits::kQLower[i];
        m_limits.m_upper[i] = WillowMPCTraits::kQUpper[i];
        m_limits.m_velocity[i] = std::min(m_config.max_joint_velocity, WillowMPCTraits::kDqLimit[i]);
        m_limits.m_acceleration[i] = m_config.max_joint_acceleration;
    }
    m_planner = std::thread([this] { planLoop(); });
    try { m_output = std::thread([this] { outputLoop(); }); }
    catch (...) { stop(); throw; }
}
MPCSession::~MPCSession() { stop(); }

void MPCSession::write(const CartesianPose& s_target) {
    if (s_target.m_motion_finished) { stop(MPCStopReason::kMotionFinished); return; }
    for (float s_value : s_target.m_T)
        if (!std::isfinite(s_value)) throw CommandException("MPC target must be finite");
    if (std::abs(s_target.m_T[3]) > 1e-5f || std::abs(s_target.m_T[7]) > 1e-5f ||
        std::abs(s_target.m_T[11]) > 1e-5f || std::abs(s_target.m_T[15] - 1) > 1e-5f)
        throw CommandException("MPC target must be a column-major homogeneous transform");
    std::lock_guard s_lock(m_write_mutex); // Serializes callers, never a worker.
    if (stopped()) throw ControlException(mpcStopReasonMessage(m_stop_reason.load()));
    if (m_targets.publish(Target{s_target}) != WL_OK) {
        fault(MPCStopReason::kInternalError);
        throw ControlException("Cannot publish MPC target");
    }
    const auto s_now = MPCClock::now();
    m_target_time_ns.store(std::chrono::duration_cast<std::chrono::nanoseconds>(
        s_now.time_since_epoch()).count(), std::memory_order_release);
    if (!m_started.load(std::memory_order_relaxed)) {
        m_first_target_time = s_now;
        m_started.store(true, std::memory_order_release);
        m_plan_wake.release();
        m_output_wake.release();
    }
}

MPCControlStatus MPCSession::status() const {
    std::lock_guard s_lock(m_status_mutex); // Multiple API readers form one consumer.
    const int s_planner_read = m_planner_status.read(m_cached_planner_status);
    const int s_output_read = m_output_status.read(m_cached_output_status);
    if ((s_planner_read != WL_OK && s_planner_read != WL_ERR_NO_DATA) ||
        (s_output_read != WL_OK && s_output_read != WL_ERR_NO_DATA))
        throw ControlException("Cannot read MPC status mailbox");
    auto s_status = m_cached_output_status;
    const auto& s_plan = m_cached_planner_status;
    s_status.m_solves = s_plan.m_solves;
    s_status.m_solver_failures = s_plan.m_solver_failures;
    s_status.m_solver_status = s_plan.m_solver_status;
    s_status.m_last_solve_ms = s_plan.m_last_solve_ms;
    s_status.m_max_solve_ms = s_plan.m_max_solve_ms;
    s_status.m_missed_planning_periods = s_plan.m_missed_planning_periods;
    s_status.m_stop_reason = m_stop_reason.load(std::memory_order_acquire);
    if (s_status.m_stop_reason != MPCStopReason::kNone) {
        s_status.m_state = s_status.m_stop_reason == MPCStopReason::kStopped ||
                           s_status.m_stop_reason == MPCStopReason::kMotionFinished
            ? MPCControlState::kStopped : MPCControlState::kFault;
    } else {
        s_status.m_state = s_status.m_outputs ? MPCControlState::kRunning :
            m_started.load(std::memory_order_acquire) ? MPCControlState::kPlanning :
            MPCControlState::kWaitingForTarget;
    }
    return s_status;
}

void MPCSession::requestStop(MPCStopReason s_reason) noexcept {
    auto s_expected = MPCStopReason::kNone;
    if (m_stop_reason.compare_exchange_strong(s_expected, s_reason, std::memory_order_acq_rel)) {
        // Only the first target and the first stop release a token, at most two.
        m_plan_wake.release();
        m_output_wake.release();
    }
}

void MPCSession::stop(MPCStopReason s_reason) noexcept {
    requestStop(s_reason == MPCStopReason::kNone ? MPCStopReason::kStopped : s_reason);
    // Only API callers join. Workers request a stop without joining themselves.
    std::lock_guard s_join(m_join_mutex);
    if (m_output.joinable()) m_output.join();
    if (m_planner.joinable()) m_planner.join();
}

void MPCSession::fault(MPCStopReason s_reason) noexcept { requestStop(s_reason); }

bool MPCSession::waitUntil(MPCClock::time_point s_time, std::counting_semaphore<2>& s_wake) {
    while (!stopped() && MPCClock::now() < s_time)
        (void)s_wake.try_acquire_until(s_time);
    return !stopped();
}

bool MPCSession::validMeasurement(const MPCMeasurement& s_m, MPCClock::time_point s_now) {
    if (!s_m.m_connected) { fault(MPCStopReason::kTransport); return false; }
    if (s_m.m_received == MPCClock::time_point{} || s_m.m_received > s_now ||
        s_now - s_m.m_received > m_config.state_timeout) {
        fault(MPCStopReason::kStaleState); return false;
    }
    if (s_m.m_state.m_errors != 0) { fault(MPCStopReason::kInvalidState); return false; }
    for (int i = 0; i < 6; ++i) {
        const double s_q = s_m.m_state.m_q[i], s_dq = s_m.m_state.m_dq[i];
        if (!std::isfinite(s_q) || !std::isfinite(s_dq) ||
            s_q < m_limits.m_lower[i] || s_q > m_limits.m_upper[i] ||
            std::abs(s_dq) > m_limits.m_velocity[i]) {
            fault(MPCStopReason::kInvalidState); return false;
        }
    }
    return true;
}

void MPCSession::planLoop() noexcept {
    MPCControlStatus s_status;
    // Publish on every exit too, including a solve that outlived a stop/deadline.
    auto s_finish = [this, &s_status](void*) {
        if (m_planner_status.publish(s_status) != WL_OK) fault(MPCStopReason::kInternalError);
    };
    std::unique_ptr<void, decltype(s_finish)> s_guard(this, s_finish);
    try {
        m_plan_wake.acquire();
        auto s_next = MPCClock::now();
        Target s_target;
        std::uint64_t s_generation = 0;
        while (!stopped()) {
            const auto s_measurement = m_plan_read();
            if (!validMeasurement(s_measurement, MPCClock::now())) return;
            const int s_read = m_targets.read(s_target);
            if (s_read != WL_OK && s_read != WL_ERR_NO_DATA) {
                fault(MPCStopReason::kInternalError); return;
            }
            MPCPlan s_plan;
            // The prediction is anchored to feedback reception, not solve completion.
            s_plan.m_origin = s_measurement.m_received;
            const auto s_begin = MPCClock::now();
            int s_result = -2;
            try { s_result = m_solve(s_measurement.m_state, s_target.m_pose, s_plan); }
            catch (...) { /* Count a failure; the executor enforces plan expiry. */ }
            const auto s_done = MPCClock::now();
            ++s_status.m_solves;
            s_status.m_solver_status = s_result;
            s_status.m_last_solve_ms = millis(s_done - s_begin);
            s_status.m_max_solve_ms = std::max(s_status.m_max_solve_ms, s_status.m_last_solve_ms);
            if (s_result != 0) ++s_status.m_solver_failures;
            if (stopped()) return;
            if (s_result == 0) {
                s_plan.m_generation = ++s_generation;
                if (m_plans.publish(s_plan) != WL_OK) { fault(MPCStopReason::kInternalError); return; }
            }
            s_next += m_config.planning_period;
            if (s_next <= s_done) {
                const auto s_missed = (s_done - s_next) / m_config.planning_period + 1;
                s_status.m_missed_planning_periods += s_missed;
                s_next += s_missed * m_config.planning_period;
            }
            if (m_planner_status.publish(s_status) != WL_OK) { fault(MPCStopReason::kInternalError); return; }
            if (!waitUntil(s_next, m_plan_wake)) return;
        }
    } catch (...) { fault(MPCStopReason::kTransport); }
}

void MPCSession::outputLoop() noexcept {
    MPCControlStatus s_status;
    auto s_finish = [this, &s_status](void*) {
        if (m_output_status.publish(s_status) != WL_OK) fault(MPCStopReason::kInternalError);
    };
    std::unique_ptr<void, decltype(s_finish)> s_guard(this, s_finish);
    try {
        TimerResolution s_resolution;
        if (!s_resolution.valid()) { fault(MPCStopReason::kOutputDeadline); return; }
        m_output_wake.acquire();
        if (stopped()) return;
        auto s_next = MPCClock::now();
        MPCPlan s_active, s_new;
        std::optional<MPCCubic> s_bridge;
        MPCClock::time_point s_bridge_start{};
        MPCJointState s_previous;
        bool s_has_previous = false;
        auto s_sample = [&](MPCClock::time_point s_t) {
            const double s_blend_time = seconds(s_t - s_bridge_start);
            if (s_bridge && s_blend_time <= MPCPlan::kDt)
                return s_bridge->sample(s_blend_time);
            return sampleMPC(s_active.m_knots, MPCPlan::kDt, seconds(s_t - s_active.m_origin));
        };
        while (!stopped()) {
            const auto s_now = MPCClock::now();
            const int s_read = m_plans.read(s_new); // NO_DATA preserves the cached plan.
            if (s_read != WL_OK && s_read != WL_ERR_NO_DATA) {
                fault(MPCStopReason::kInternalError); return;
            }
            const auto s_target_time = MPCClock::time_point(std::chrono::duration_cast<MPCClock::duration>(
                std::chrono::nanoseconds(m_target_time_ns.load(std::memory_order_acquire))));
            s_status.m_max_output_lateness_ms = std::max(s_status.m_max_output_lateness_ms,
                millis(s_now - s_next));
            if (s_now - s_next > m_config.output_lateness_limit) {
                fault(MPCStopReason::kOutputDeadline); return;
            }
            if (s_now - s_target_time > m_config.target_timeout) {
                fault(MPCStopReason::kStaleTarget); return;
            }
            const auto s_measurement = m_output_read();
            const auto s_checked = MPCClock::now();
            if (!validMeasurement(s_measurement, s_checked)) return;
            if (s_active.m_generation && s_checked - s_active.m_origin > m_config.plan_timeout &&
                s_new.m_generation == s_active.m_generation) {
                fault(MPCStopReason::kStalePlan); return;
            }
            if (s_new.m_generation != s_active.m_generation) {
                if (s_checked - s_new.m_origin > m_config.plan_timeout) {
                    fault(MPCStopReason::kStalePlan); return;
                }
                for (std::size_t i = 0; i + 1 < s_new.m_knots.size(); ++i) {
                    if (!MPCCubic(s_new.m_knots[i], s_new.m_knots[i+1], MPCPlan::kDt).within(m_limits)) {
                        std::fprintf(stderr,"INVALID_PREDICTION segment=%zu\n",i);
                        traceCurve("prediction",s_new.m_knots[i],s_new.m_knots[i+1],m_limits,MPCPlan::kDt);
                        tracePlan("candidate",s_new,s_now);
                        fault(MPCStopReason::kInvalidTrajectory); return;
                    }
                }
                MPCJointState s_anchor;
                if (s_active.m_generation) s_anchor = s_sample(s_now);
                else {
                    // First handoff starts at the measured position/velocity.
                    for (int i = 0; i < 6; ++i) {
                        s_anchor.m_q[i] = s_measurement.m_state.m_q[i];
                        s_anchor.m_dq[i] = s_measurement.m_state.m_dq[i];
                    }
                }
                const auto s_end = sampleMPC(s_new.m_knots, MPCPlan::kDt,
                    seconds(s_now - s_new.m_origin) + MPCPlan::kDt);
                MPCCubic s_transition(s_anchor, s_end, MPCPlan::kDt);
                if (!s_transition.within(m_limits)) {
                    traceCurve("transition",s_anchor,s_end,m_limits,MPCPlan::kDt);
                    tracePlan("candidate",s_new,s_now);
                    tracePlan("active",s_active,s_now);
                    fault(MPCStopReason::kInvalidTrajectory); return;
                }
                s_active = s_new;
                s_bridge = s_transition;
                s_bridge_start = s_now;
                ++s_status.m_plan_switches;
            }
            if (s_active.m_generation) {
                const auto s_ref = s_sample(s_now);
                JointPVT s_cmd;
                for (int i = 0; i < 6; ++i) {
                    if (std::abs(s_ref.m_q[i] - s_measurement.m_state.m_q[i]) > m_config.max_tracking_error) {
                        fault(MPCStopReason::kTrackingError); return;
                    }
                    s_cmd.m_q[i] = static_cast<float>(s_ref.m_q[i]);
                    // PVT takes a speed ceiling. The 2 ms displacement plus
                    // bounded feedback catch-up are both covered, never signed FF.
                    double s_speed = std::max(std::abs(s_ref.m_dq[i]),
                        std::abs(s_ref.m_q[i] - s_measurement.m_state.m_q[i]) / 0.020);
                    if (s_has_previous)
                        s_speed = std::max(s_speed, std::abs(s_ref.m_q[i] - s_previous.m_q[i]) / 0.002);
                    s_cmd.m_dq_limit[i] = static_cast<float>(std::min(m_limits.m_velocity[i],
                        std::max(1e-4, s_speed * m_config.velocity_excitation)));
                    s_cmd.m_current_limit_norm[i] = m_config.current_limit_norm;
                }
                const auto s_submit_time = MPCClock::now();
                s_status.m_max_output_lateness_ms = std::max(s_status.m_max_output_lateness_ms,
                    millis(s_submit_time - s_next));
                if (s_submit_time - s_next > m_config.output_lateness_limit) {
                    fault(MPCStopReason::kOutputDeadline); return;
                }
                if (stopped()) return;
                m_send(s_cmd);
                s_previous = s_ref;
                s_has_previous = true;
                s_status.m_state = MPCControlState::kRunning;
                ++s_status.m_outputs;
                s_status.m_state_age_ms = millis(s_checked - s_measurement.m_received);
                s_status.m_plan_age_ms = millis(s_checked - s_active.m_origin);
                for (int i = 0; i < 6; ++i) {
                    s_status.m_reference_q[i] = s_cmd.m_q[i];
                    s_status.m_reference_dq[i] = static_cast<float>(s_ref.m_dq[i]);
                }
            } else if (s_checked - m_first_target_time > m_config.plan_timeout) {
                fault(MPCStopReason::kStalePlan); return;
            }
            const auto s_done = MPCClock::now();
            s_status.m_max_output_lateness_ms = std::max(s_status.m_max_output_lateness_ms,
                millis(s_done - s_next));
            if (s_done - s_next > m_config.output_lateness_limit) {
                fault(MPCStopReason::kOutputDeadline); return;
            }
            s_next += m_config.output_period;
            if (s_next <= s_done) {
                const auto s_missed = (s_done - s_next) / m_config.output_period + 1;
                s_status.m_missed_output_periods += s_missed;
                s_next += s_missed * m_config.output_period;
            }
            if (m_output_status.publish(s_status) != WL_OK) { fault(MPCStopReason::kInternalError); return; }
            if (!waitUntil(s_next, m_output_wake)) return;
        }
    } catch (...) { fault(MPCStopReason::kTransport); }
}
} // namespace florid::detail
