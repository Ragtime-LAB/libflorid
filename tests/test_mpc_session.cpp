#include "florid/detail/MPCSession.hpp"
#include "florid/mpc/CartesianMPC.hpp"
#include "florid/Model.hpp"
#include "florid/traits/WillowTraits.hpp"
#include "WillowMPCTraits.hpp"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <deque>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace std::chrono_literals;
using namespace florid;
using namespace florid::detail;
using Clock = MPCClock;
void require(bool s_ok, const char* s_message) { if (!s_ok) throw std::runtime_error(s_message); }
template <typename F> void until(F s_predicate, const char* s_message, std::chrono::milliseconds s_timeout = 500ms) {
    const auto s_end = Clock::now() + s_timeout;
    while (!s_predicate()) {
        if (Clock::now() >= s_end) throw std::runtime_error(s_message);
        std::this_thread::sleep_for(1ms);
    }
}
ArmState state() {
    ArmState s;
    const float s_q[6]{0.1f, 1.5f, 0.3f, 0.1f, 0.2f, 0.1f};
    std::copy_n(s_q, 6, s.m_q);
    s.m_seq = 1;
    return s;
}
CartesianPose pose(const ArmState& s = state()) {
    CartesianPose s_target;
    Model<WillowTraits>{}.forwardKinematics(s.m_q, s_target.m_T);
    return s_target;
}
MPCMeasurement fresh() { return {state(), Clock::now(), true}; }
int hold(const ArmState& s, const CartesianPose&, MPCPlan& p) {
    for (auto& k : p.m_knots)
        for (int i = 0; i < 6; ++i) { k.m_q[i] = s.m_q[i]; k.m_dq[i] = 0; }
    return 0;
}
void ignore(const JointPVT&) {}
MPCControlConfig config() {
    MPCControlConfig c;
    // Allow ordinary CI scheduling jitter; the production default remains 10 ms.
    c.output_lateness_limit = 30ms;
    c.plan_timeout = 80ms;
    return c;
}
void expectFault(MPCSession& s, MPCStopReason r) {
    until([&] { return s.status().m_state == MPCControlState::kFault; }, "session did not fault");
    if (s.status().m_stop_reason != r)
        throw std::runtime_error(std::string("wrong fault: ") + mpcStopReasonMessage(s.status().m_stop_reason));
    s.stop();
}

void testCubic() {
    MPCJointLimits l;
    l.m_lower.fill(-10); l.m_upper.fill(10); l.m_velocity.fill(10); l.m_acceleration.fill(100);
    MPCJointState a, b;
    a.m_q.fill(0.1); a.m_dq.fill(0.2);
    b.m_q.fill(0.15); b.m_dq.fill(-0.1);
    MPCCubic c(a, b, 0.1);
    require(c.within(l), "reasonable cubic rejected");
    for (int i = 0; i < 6; ++i) {
        require(std::abs(c.sample(0).m_q[i] - a.m_q[i]) < 1e-12 &&
                std::abs(c.sample(0).m_dq[i] - a.m_dq[i]) < 1e-12 &&
                std::abs(c.sample(0.1).m_q[i] - b.m_q[i]) < 1e-12 &&
                std::abs(c.sample(0.1).m_dq[i] - b.m_dq[i]) < 1e-12, "Hermite endpoints lost q/dq");
    }
    // q endpoints are inside; opposite endpoint velocities overshoot in between.
    a = {}; b = {}; a.m_dq[0] = 1; b.m_dq[0] = -1;
    l.m_upper[0] = 0.1;
    require(!MPCCubic(a, b, 1).within(l), "interior position overshoot accepted");
    l.m_upper[0] = 10;
    a = {}; b = {}; b.m_q[0] = 1;
    l.m_velocity[0] = 1;
    require(!MPCCubic(a, b, 1).within(l), "interior velocity peak accepted");
    l.m_velocity[0] = 10; l.m_acceleration[0] = 5;
    require(!MPCCubic(a, b, 1).within(l), "acceleration bound ignored");
    b.m_q[0] = std::numeric_limits<double>::quiet_NaN();
    require(!MPCCubic(a, b, 1).within(l), "NaN spline accepted");
    require(!MPCCubic(a, a, 0).within(l), "zero-duration spline accepted");

    // The plan is sampled in elapsed feedback time, including a 13 ms solve.
    MPCPlan p;
    for (std::size_t i = 0; i < p.m_knots.size(); ++i) {
        p.m_knots[i].m_q[0] = i * 0.02 * 0.1;
        p.m_knots[i].m_dq[0] = 0.1;
    }
    require(std::abs(sampleMPC(p.m_knots, 0.02, 0.013).m_q[0] - 0.0013) < 1e-12,
            "elapsed solve time was discarded");
    auto anchor = sampleMPC(p.m_knots, 0.02, 0.031);
    auto end = sampleMPC(p.m_knots, 0.02, 0.051);
    MPCCubic blend(anchor, end, 0.02);
    require(std::abs(blend.sample(0).m_dq[0] - anchor.m_dq[0]) < 1e-12 &&
            std::abs(blend.sample(0.02).m_dq[0] - end.m_dq[0]) < 1e-12,
            "handoff lost C1 continuity");
}

void testSlowSolverAndLifecycle() {
    std::atomic<int> calls{0};
    std::atomic<bool> slow_started{false};
    MPCSession s(config(), fresh, ignore, [&](const auto& x, const auto& t, auto& p) {
        if (++calls == 2) { slow_started = true; std::this_thread::sleep_for(35ms); }
        return hold(x, t, p);
    });
    require(s.status().m_outputs == 0, "session commanded before receiving a target");
    s.write(pose());
    until([&] { return slow_started.load(); }, "second solve did not start");
    const auto before = s.status().m_outputs;
    std::this_thread::sleep_for(25ms);
    require(s.status().m_outputs >= before + 5, "slow solve blocked the output worker");
    until([&] { return s.status().m_missed_planning_periods > 0; }, "missed solve period not counted");
    s.stop();
    const auto stopped = s.status();
    require(stopped.m_state == MPCControlState::kStopped && stopped.m_max_solve_ms >= 30,
            "stop or solver timing status incorrect");
    std::this_thread::sleep_for(10ms);
    require(s.status().m_outputs == stopped.m_outputs, "output continued after stop returned");
    bool threw = false;
    try { s.write(pose()); } catch (const ControlException&) { threw = true; }
    require(threw, "stopped session accepted a new target");

    MPCSession finished(config(), fresh, ignore, hold);
    finished.write(pose());
    until([&] { return finished.status().m_outputs >= 3; }, "finished session did not run");
    finished.write(CartesianPose::MotionFinished(pose()));
    require(finished.status().m_stop_reason == MPCStopReason::kMotionFinished, "MotionFinished not handled");
    std::atomic<unsigned> outputs{};
    {
        MPCSession scoped(config(), fresh, [&](auto&) { ++outputs; }, hold);
        scoped.write(pose());
        until([&] { return outputs.load() > 2; }, "scoped session did not run");
    }
    const auto count = outputs.load();
    std::this_thread::sleep_for(10ms);
    require(outputs == count, "destructor left a live output worker");

    MPCSession concurrent(config(), fresh, ignore, hold);
    concurrent.write(pose());
    until([&] { return concurrent.status().m_outputs >= 3; }, "concurrent session did not run");
    std::atomic<bool> rejected{};
    std::jthread writer([&](std::stop_token token) {
        while (!token.stop_requested()) {
            try { concurrent.write(pose()); }
            catch (const ControlException&) { rejected = true; return; }
            std::this_thread::sleep_for(1ms);
        }
    });
    std::jthread stopper([&] { concurrent.stop(); });
    concurrent.stop();
    until([&] { return rejected.load(); }, "write/stop race revived a session");
}

void testConcurrentExchange() {
    std::atomic<bool> coherent{true};
    MPCSession session(config(), fresh, ignore, [&](const auto& x, const auto& t, auto& p) {
        if (t.m_T[12] != t.m_T[14] || t.m_T[13] != -t.m_T[12]) coherent = false;
        return hold(x, t, p);
    });
    auto target = pose();
    target.m_T[12] = target.m_T[13] = target.m_T[14] = 0;
    session.write(target);
    auto writer = [&](std::stop_token token) {
        auto t = target;
        for (int i = 1; !token.stop_requested(); ++i) {
            t.m_T[12] = t.m_T[14] = (i % 1000) * 0.0001f;
            t.m_T[13] = -t.m_T[12];
            session.write(t);
            std::this_thread::sleep_for(1ms);
        }
    };
    auto reader = [&](std::stop_token token) {
        MPCControlStatus previous;
        while (!token.stop_requested()) {
            const auto current = session.status();
            if (current.m_outputs < previous.m_outputs || current.m_solves < previous.m_solves ||
                current.m_state == MPCControlState::kFault) coherent = false;
            if (current.m_outputs) for (int i = 0; i < 6; ++i)
                if (current.m_reference_q[i] != state().m_q[i] || current.m_reference_dq[i] != 0)
                    coherent = false;
            previous = current;
            std::this_thread::sleep_for(100us);
        }
    };
    std::jthread a(writer), b(writer), c(reader), d(reader);
    std::this_thread::sleep_for(300ms);
    a.request_stop(); b.request_stop(); c.request_stop(); d.request_stop();
    a.join(); b.join(); c.join(); d.join();
    session.stop();
    const auto final = session.status();
    require(coherent && final.m_outputs >= 100 && final.m_solves >= 10,
            "concurrent target/status exchange tore data or stalled output");
}

void testFaults() {
    {
        MPCSession s(config(), fresh, ignore, [](auto&, auto&, auto&) { return 4; });
        s.write(pose()); expectFault(s, MPCStopReason::kStalePlan);
        require(s.status().m_solver_failures > 0 && s.status().m_outputs == 0,
                "failed solve became a usable plan");
    }
    {
        MPCSession s(config(), fresh, ignore, [](const auto& x, const auto& t, auto& p) {
            std::this_thread::sleep_for(100ms); return hold(x, t, p);
        });
        s.write(pose()); expectFault(s, MPCStopReason::kStalePlan);
        require(s.status().m_outputs == 0, "late result revived an expired session");
    }
    {
        std::atomic<bool> fail{};
        MPCSession s(config(), fresh, ignore, [&](const auto& x, const auto& t, auto& p) {
            return fail ? 4 : hold(x, t, p);
        });
        s.write(pose());
        until([&] { return s.status().m_outputs >= 4; }, "initial valid plan failed");
        fail = true;
        const auto before = s.status().m_outputs;
        expectFault(s, MPCStopReason::kStalePlan);
        require(s.status().m_outputs > before, "previous valid plan was not used during a brief solve failure");
    }
    {
        MPCSession s(config(), fresh, ignore, hold);
        s.write(pose()); expectFault(s, MPCStopReason::kStaleTarget);
    }
    {
        const auto received = Clock::now();
        MPCSession s(config(), [=] { return MPCMeasurement{state(), received, true}; }, ignore, hold);
        s.write(pose()); expectFault(s, MPCStopReason::kStaleState);
    }
    {
        MPCSession s(config(), [] { auto x = fresh(); x.m_state.m_q[2] = NAN; return x; }, ignore, hold);
        s.write(pose()); expectFault(s, MPCStopReason::kInvalidState);
    }
    {
        MPCSession s(config(), [] { auto x = fresh(); x.m_connected = false; return x; }, ignore, hold);
        s.write(pose()); expectFault(s, MPCStopReason::kTransport);
    }
    {
        MPCSession s(config(), fresh, [](auto&) { throw std::runtime_error("link failed"); }, hold);
        s.write(pose()); expectFault(s, MPCStopReason::kTransport);
    }
    {
        std::atomic<unsigned> sent{};
        MPCSession s(config(), fresh, [&](auto&) {
            ++sent;
            std::this_thread::sleep_for(40ms);
        }, hold);
        s.write(pose()); expectFault(s, MPCStopReason::kOutputDeadline);
        require(sent == 1, "blocked output did not stop at its deadline");
    }
    {
        MPCSession s(config(), fresh, ignore, [](const auto& x, const auto& t, auto& p) {
            hold(x, t, p); p.m_knots[2].m_q[0] = NAN; return 0;
        });
        s.write(pose()); expectFault(s, MPCStopReason::kInvalidTrajectory);
    }
    {
        auto c = config(); c.max_tracking_error = 1e-5;
        MPCSession s(c, fresh, ignore, [](const auto& x, const auto& t, auto& p) {
            hold(x, t, p);
            for (std::size_t i = 0; i < p.m_knots.size(); ++i) {
                p.m_knots[i].m_q[0] += 0.01 * i * MPCPlan::kDt;
                p.m_knots[i].m_dq[0] = 0.01;
            }
            return 0;
        });
        s.write(pose()); expectFault(s, MPCStopReason::kTrackingError);
    }
}

// A delayed first-order PVT position servo with a speed ceiling. Its response
// is deliberately not the optimized torque-driven plant, nor an ideal q jump.
class LaggedPlant {
public:
    MPCMeasurement read() {
        std::lock_guard lock(m_mutex);
        advance();
        const auto now = Clock::now();
        m_history.push_back({m_state, now, true});
        while (m_history.size() > 1 && m_history[1].m_received <= now - 4ms) m_history.pop_front();
        return m_history.front();
    }
    void send(const JointPVT& cmd) {
        std::lock_guard lock(m_mutex);
        advance();
        if (m_commands) {
            for (int i = 0; i < 6; ++i)
                m_max_step = std::max(m_max_step, double(std::abs(cmd.m_q[i] - m_command.m_q[i])));
        }
        for (int i = 0; i < 6; ++i)
            require(std::isfinite(cmd.m_q[i]) && cmd.m_dq_limit[i] > 0 && cmd.m_dq_limit[i] <= 0.5f,
                    "invalid PVT command in lagged simulation");
        m_command = cmd;
        ++m_commands;
    }
    double maxStep() { std::lock_guard lock(m_mutex); return m_max_step; }
private:
    void advance() {
        const auto now = Clock::now();
        double dt = std::chrono::duration<double>(now - m_time).count();
        while (dt > 1e-9 && m_commands) {
            const auto h = std::min(dt, 0.0002);
            for (int i = 0; i < 6; ++i) {
                const double v = std::clamp((m_command.m_q[i] - m_state.m_q[i]) / 0.012,
                    -double(m_command.m_dq_limit[i]), double(m_command.m_dq_limit[i]));
                m_state.m_q[i] += v * h;
                m_state.m_dq[i] = v;
            }
            dt -= h;
        }
        ++m_state.m_seq;
        m_time = now;
    }
    std::mutex m_mutex;
    ArmState m_state{state()};
    JointPVT m_command{};
    Clock::time_point m_time{Clock::now()};
    std::deque<MPCMeasurement> m_history;
    std::size_t m_commands{};
    double m_max_step{};
};

void testExampleMotionWithLag() {
    LaggedPlant plant;
    CartesianMPCSolver<WillowMPCTraits> solver;
    solver.setVelocityLimit(0.5f);
    auto c = config(); c.max_joint_velocity = 0.5f; c.max_joint_acceleration = 10;
    MPCSession session(c, [&] { return plant.read(); }, [&](auto& cmd) { plant.send(cmd); },
        [&](const ArmState& x, const CartesianPose& target, MPCPlan& p) {
            const auto begin = Clock::now();
            solver.predict(x.m_q, x.m_dq, target.m_T, p.m_knots);
            const double ms = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
            if (ms > 20) std::fprintf(stderr, "simulation solve %.3f ms (q0=%.7f dq0=%.7f status=%d)\n",
                ms, x.m_q[0], x.m_dq[0], solver.lastStatus());
            return solver.lastStatus();
        });
    auto initial = pose();
    const auto start = Clock::now();
    auto next = start;
    double max_x = initial.m_T[12];
    while (Clock::now() - start < 5500ms) {
        auto target = initial;
        const double t = std::chrono::duration<double>(Clock::now() - start).count();
        if (t > 0.5 && t < 4.5) target.m_T[12] += 0.005 * (1 - std::cos((t - 0.5) * 1.5707963267948966));
        const auto status = session.status();
        if (status.m_state == MPCControlState::kFault) {
            std::fprintf(stderr, "lagged fault at %.3f s: %s, solver=%d, solve max=%.3f ms, solves=%llu, outputs=%llu, age=%.3f ms, lateness=%.3f ms, skipped=%llu/%llu\n",
                t, mpcStopReasonMessage(status.m_stop_reason), status.m_solver_status, status.m_max_solve_ms,
                static_cast<unsigned long long>(status.m_solves), static_cast<unsigned long long>(status.m_outputs),
                status.m_plan_age_ms, status.m_max_output_lateness_ms,
                static_cast<unsigned long long>(status.m_missed_planning_periods),
                static_cast<unsigned long long>(status.m_missed_output_periods));
            throw std::runtime_error("example trajectory faulted with servo lag");
        }
        session.write(target);
        max_x = std::max(max_x, double(pose(plant.read().m_state).m_T[12]));
        next += 20ms;
        if (next < Clock::now()) next = Clock::now() + 20ms;
        std::this_thread::sleep_until(next);
    }
    session.stop();
    const auto s = session.status();
    const auto final_pose = pose(plant.read().m_state);
    double err = 0;
    for (int i = 12; i < 15; ++i) err += std::pow(final_pose.m_T[i] - initial.m_T[i], 2);
    err = std::sqrt(err);
    std::printf("Lagged PVT: peak X=%.3f mm, final error=%.3f mm, solves=%llu outputs=%llu, "
                "solve max=%.3f ms, max step=%.6f rad\n", (max_x - initial.m_T[12])*1000, err*1000,
                static_cast<unsigned long long>(s.m_solves), static_cast<unsigned long long>(s.m_outputs),
                s.m_max_solve_ms, plant.maxStep());
    require(s.m_solver_failures == 0 && s.m_solves > 200 && s.m_outputs > s.m_solves * 8,
            "multi-rate solve/output cadence was not maintained");
    require(max_x - initial.m_T[12] > 0.008 && max_x - initial.m_T[12] < 0.012,
            "1 cm example did not reach its excursion with servo lag");
    require(err < 0.0005, "example did not return within 0.5 mm with servo lag");
    require(plant.maxStep() < 0.002, "trajectory handoff caused a joint position jump");
}
}
int main() {
    try {
        testCubic();
        testSlowSolverAndLifecycle();
        testConcurrentExchange();
        testFaults();
        testExampleMotionWithLag();
        std::puts("MPC session tests passed");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what()); return 1;
    }
}
