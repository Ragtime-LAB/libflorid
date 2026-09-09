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

void requireSameState(const MPCJointState& s_a, const MPCJointState& s_b) {
    for (int i = 0; i < 6; ++i)
        require(std::abs(s_a.m_q[i] - s_b.m_q[i]) < 1e-12 &&
                std::abs(s_a.m_dq[i] - s_b.m_dq[i]) < 1e-12, "transition lost C1 continuity");
}

void testWindowsTransition() {
    // Native MSVC failure: prediction segments were valid, but the 20 ms handoff
    // needed 10.580102 rad/s^2 on joint 2. Keep the captured doubles and plan age.
    // https://github.com/Ragtime-LAB/libflorid/actions/runs/34377417068
    MPCJointLimits s_limits;
    s_limits.m_lower = {-3.1400001049041748, 0, 0, -1.2999999523162842, -1.5700000524520874, -1.5700000524520874};
    s_limits.m_upper = {3.1400001049041748, 3.1400001049041748, 3.1400001049041748, 1.2999999523162842, 1.5700000524520874, 1.5700000524520874};
    s_limits.m_velocity.fill(0.5); s_limits.m_acceleration.fill(10);
    const MPCJointState s_anchor{
        {0.09765782315964168, 1.5117579463045068, 0.30788505895886603, 0.089907266727183494, 0.19951800699227404, 0.10001564463135885},
        {-0.0077748664555391742, 0.040898633297347074, 0.029268941197865203, -0.031315605117026926, -0.0016724162524378782, 5.6443359054458225e-05}
    };
    const std::array<MPCJointState, 6> s_knots{{
        {
            {0.097852610051631927, 1.5107414722442627, 0.30715674161911011, 0.090719617903232574, 0.1995597630739212, 0.10001423954963684},
            {-0.0021296243648976088, 0.0096062822267413139, 0.0028063852805644274, -0.0089779496192932129, -0.00051905709551647305, 1.3659398064191919e-05}
        },
        {
            {0.097768009826218791, 1.5111442659616823, 0.30736192117066202, 0.090353889215255459, 0.19954067958895269, 0.10001485682434598},
            {-0.0063196266600503365, 0.0307543364166219, 0.017788488806078528, -0.027422662032866855, -0.0013523328451140577, 4.3411354803774588e-06}
        },
        {
            {0.097640062472062486, 1.5117878803686446, 0.3077564073112502, 0.089806081728966466, 0.19951250869804518, 0.1000155908330215},
            {-0.0064579471840674942, 0.03374573269189373, 0.021771458427352453, -0.027114441609234839, -0.0014015007219856112, 1.759256787396385e-06}
        },
        {
            {0.097510501686477832, 1.5124599188239887, 0.30817894317124878, 0.089273060160937237, 0.19948346002657261, 0.10001627579796939},
            {-0.0064814575890468993, 0.033604304253663064, 0.020591406149875712, -0.02595891639449496, -0.0014386412110127477, 1.0352144237324069e-06}
        },
        {
            {0.097382927559653429, 1.5130855510604111, 0.30853208212534239, 0.088770142665239873, 0.19945377958711236, 0.10001688000056357},
            {-0.0062647856190208373, 0.02909294199178537, 0.014826110286677434, -0.024115367206685081, -0.0014747063659523746, 2.2170979996374482e-06}
        },
        {
            {0.097319937021595998, 1.5133779695638756, 0.30868274424116865, 0.08852615433004199, 0.19943872200939838, 0.10001716323537314},
            {-3.042008218392113e-05, 0.00021071428707767107, 0.00029011858529544483, -0.00017583305892893982, -7.0537275088597005e-06, -3.9900662313240439e-08}
        },
    }};
    constexpr double s_age = 0.0210323;
    for (std::size_t i = 0; i + 1 < s_knots.size(); ++i)
        require(MPCCubic(s_knots[i], s_knots[i+1], 0.020).within(s_limits),
                "captured prediction itself exceeds limits");
    require(!MPCCubic(s_anchor, sampleMPC(s_knots, 0.020, s_age + 0.020), 0.020).within(s_limits),
            "captured Windows handoff no longer reproduces the acceleration violation");

    const auto s_bridge = makeMPCTransition(s_anchor, s_knots, 0.020, s_age, 0.080, s_limits);
    require(s_bridge && std::abs(s_bridge->duration() - 0.022) < 1e-12 &&
            s_bridge->within(s_limits), "no shortest feasible Windows handoff found");
    requireSameState(s_bridge->sample(0), s_anchor);
    requireSameState(s_bridge->sample(s_bridge->duration()),
                     sampleMPC(s_knots, 0.020, s_age + s_bridge->duration()));
    require(!makeMPCTransition(s_anchor, s_knots, 0.020, s_age, s_age + 0.021, s_limits),
            "transition exceeded the remaining plan lifetime");
}

void testTransitionBoundsAndRetargeting() {
    MPCJointLimits s_limits;
    s_limits.m_lower.fill(-10); s_limits.m_upper.fill(10);
    s_limits.m_velocity.fill(0.5); s_limits.m_acceleration.fill(10);
    std::array<MPCJointState, 6> s_knots{};
    MPCJointState s_anchor;
    const auto s_hold = makeMPCTransition(s_anchor, s_knots, 0.020, 0, 0.080, s_limits);
    require(s_hold && s_hold->duration() == 0.020, "ordinary hold unnecessarily extended");
    require(!makeMPCTransition(s_anchor, s_knots, 0.020, 0.061, 0.080, s_limits),
            "transition ignored plan expiry");
    require(!makeMPCTransition(s_anchor, s_knots, 0.004, 0.001, 0.080, s_limits),
            "transition sampled beyond the prediction horizon");
    require(!makeMPCTransition(s_anchor, s_knots, 0.020, -0.001, 0.080, s_limits) &&
            !makeMPCTransition(s_anchor, s_knots, 0.020, NAN, 0.080, s_limits),
            "invalid prediction time accepted");

    // This motion needs >40 ms even though there is enough prediction/lifetime.
    s_anchor.m_q[0] = 0.003;
    require(MPCCubic(s_anchor, s_knots[0], 0.044).within(s_limits),
            "duration-cap fixture is not feasible after 40 ms");
    require(!makeMPCTransition(s_anchor, s_knots, 0.020, 0, 0.080, s_limits),
            "transition search exceeded its 40 ms cap");

    // Replan every 20 ms on a moving reference. The first bridge takes >20 ms,
    // so the next update must start from the middle of it, not measured state.
    s_anchor.m_q[0] = 0.101;
    s_anchor.m_dq[0] = 0.04;
    for (int s_update = 0; s_update < 50; ++s_update) {
        for (std::size_t i = 0; i < s_knots.size(); ++i) {
            s_knots[i].m_q[0] = 0.1 + 0.04 * (s_update * 0.020 + i * 0.020);
            s_knots[i].m_dq[0] = 0.04;
        }
        const auto s_bridge = makeMPCTransition(s_anchor, s_knots, 0.020, 0, 0.080, s_limits);
        require(s_bridge && s_bridge->within(s_limits), "successive handoff became infeasible");
        if (s_update == 0) require(s_bridge->duration() > 0.020, "unfinished handoff not exercised");
        requireSameState(s_bridge->sample(0), s_anchor);
        requireSameState(s_bridge->sample(s_bridge->duration()),
                         sampleMPC(s_knots, 0.020, s_bridge->duration()));
        s_anchor = s_bridge->sample(0.020);
    }
    require(std::abs(s_anchor.m_q[0] - 0.14) < 1e-9 &&
            std::abs(s_anchor.m_dq[0] - 0.04) < 1e-9, "repeated retargeting accumulated reference lag");
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
        testWindowsTransition();
        testTransitionBoundsAndRetargeting();
        testSlowSolverAndLifecycle();
        testConcurrentExchange();
        testFaults();
        testExampleMotionWithLag();
        std::puts("MPC session tests passed");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what()); return 1;
    }
}
