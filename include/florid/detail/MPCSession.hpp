#pragma once

#include "florid/mpc/MPCControl.hpp"
#include "florid/detail/MPCTrajectory.hpp"
#include "florid/detail/LatestValue.hpp"
#include <atomic>
#include <mutex>
#include <semaphore>
#include <thread>

namespace florid::detail {
using MPCClock = std::chrono::steady_clock;
struct MPCMeasurement {
    ArmState m_state{};
    MPCClock::time_point m_received{};
    bool m_connected{true};
    std::uint64_t m_generation{};
};
struct MPCPlan {
    static constexpr double kDt = 0.020;
    std::array<MPCJointState, 6> m_knots{};
    MPCClock::time_point m_origin{};
    std::uint64_t m_generation{};
};

// One solver owner and one output owner. All data exchanged with those workers
// uses preallocated SPSC mailboxes. Caller locks are never taken by a worker.
class MPCSession {
public:
    using Read = std::function<MPCMeasurement()>;
    using Send = std::function<void(const JointPVT&)>;
    using Solve = std::function<int(const ArmState&, const CartesianPose&, MPCPlan&)>;
    MPCSession(MPCControlConfig s_config, Read s_read, Send s_send, Solve s_solve);
    MPCSession(MPCControlConfig s_config, Read s_plan_read, Read s_output_read,
               Send s_send, Solve s_solve);
    ~MPCSession();
    MPCSession(const MPCSession&) = delete;
    MPCSession& operator=(const MPCSession&) = delete;
    void write(const CartesianPose& s_target);
    void stop(MPCStopReason s_reason = MPCStopReason::kStopped) noexcept;
    MPCControlStatus status() const;
    bool stopped() const noexcept { return m_stop_reason.load(std::memory_order_acquire) != MPCStopReason::kNone; }
    static void validateConfig(const MPCControlConfig& s_config);

private:
    void planLoop() noexcept;
    void outputLoop() noexcept;
    void fault(MPCStopReason s_reason) noexcept;
    bool waitUntil(MPCClock::time_point s_time, std::counting_semaphore<2>& s_wake);
    void requestStop(MPCStopReason s_reason) noexcept;
    bool validMeasurement(const MPCMeasurement& s_measurement, MPCClock::time_point s_now);
    MPCControlConfig m_config;
    MPCJointLimits m_limits;
    Read m_plan_read, m_output_read;
    Send m_send;
    Solve m_solve;
    struct Target { CartesianPose m_pose{}; };
    LatestValue<Target> m_targets;
    LatestValue<MPCPlan> m_plans;
    mutable LatestValue<MPCControlStatus> m_planner_status, m_output_status;
    mutable MPCControlStatus m_cached_planner_status{}, m_cached_output_status{};
    std::mutex m_write_mutex;
    mutable std::mutex m_status_mutex;
    std::mutex m_join_mutex;
    std::counting_semaphore<2> m_plan_wake{0}, m_output_wake{0};
    std::atomic<MPCStopReason> m_stop_reason{MPCStopReason::kNone};
    std::atomic<bool> m_started{false};
    std::atomic<std::int64_t> m_target_time_ns{0};
    MPCClock::time_point m_first_target_time{}; // immutable after first wake
    std::thread m_planner, m_output;
};
} // namespace florid::detail
