#include <florid/Arm.hpp>
#include <florid/core/ActiveControl.hpp>

#include <algorithm>

namespace florid
{
    struct ActiveControl<Torques>::Impl
    {
        using Clock = std::chrono::steady_clock;

        Impl(Arm& arm, double rate_hz, double command_timeout_ms)
            : arm(&arm)
            , period_us(rateToPeriodUs(rate_hz))
            , command_timeout_us(timeoutToUs(command_timeout_ms))
        {
            latest_state = arm.updateThreadSafe();
            state_time = Clock::now();
        }

        ~Impl()
        {
            stop();
        }

        static uint64_t rateToPeriodUs(double rate_hz)
        {
            if (rate_hz <= 0.0) rate_hz = 500.0;
            return static_cast<uint64_t>(1000000.0 / rate_hz);
        }

        static uint64_t timeoutToUs(double timeout_ms)
        {
            if (timeout_ms < 0.0) timeout_ms = 0.0;
            return static_cast<uint64_t>(timeout_ms * 1000.0);
        }

        static uint64_t elapsedUs(Clock::time_point now, Clock::time_point then)
        {
            return static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(now - then).count());
        }

        void start()
        {
            if (!arm) return;
            thread = std::thread([this] { run(); });
        }

        void stop()
        {
            stop_requested.store(true, std::memory_order_release);
            wakeup.notify_all();
            if (thread.joinable())
                thread.join();
        }

        void run()
        {
            if (!arm) return;

            arm->beginControlSession();

            const auto period = std::chrono::microseconds(period_us);
            auto next_tick = Clock::now();
            auto previous_tick = next_tick;
            uint64_t loop_count = 0;
            uint64_t period_sum_us = 0;
            uint64_t stale_ticks = 0;
            const auto start_time = next_tick;

            while (!stop_requested.load(std::memory_order_acquire)
                   && !finished.load(std::memory_order_acquire))
            {
                next_tick += period;
                ArmState state = arm->updateThreadSafe();
                auto now = Clock::now();
                {
                    std::lock_guard<std::mutex> lock(state_mutex);
                    latest_state = state;
                    state_time = now;
                }

                Torques command{};
                bool should_send = false;
                uint64_t command_age_us = 0;

                if (has_command.load(std::memory_order_acquire))
                {
                    std::lock_guard<std::mutex> lock(command_mutex);
                    now = Clock::now();
                    command_age_us = elapsedUs(now, command_write_time);
                    if (command_age_us <= command_timeout_us)
                    {
                        command = latest_command;
                        should_send = true;
                    }
                    else
                    {
                        ++stale_ticks;
                    }
                }

                if (should_send)
                    arm->sendCommand(command);

                now = Clock::now();
                const uint64_t actual_period_us = elapsedUs(now, previous_tick);
                previous_tick = now;
                ++loop_count;
                period_sum_us += actual_period_us;

                {
                    std::lock_guard<std::mutex> lock(diag_mutex);
                    const auto elapsed_total_us = std::max<uint64_t>(1, elapsedUs(now, start_time));
                    diag.actual_hz = static_cast<double>(loop_count) * 1000000.0
                                     / static_cast<double>(elapsed_total_us);
                    diag.period_us_avg = period_sum_us / loop_count;
                    diag.period_us_max = std::max(diag.period_us_max, actual_period_us);
                    if (actual_period_us > period_us)
                        ++diag.overrun_count;
                    diag.command_age_us = command_age_us;
                    if (should_send)
                    {
                        ++diag.sent_count;
                        diag.last_sdk_timestamp_us = arm->lastCommandTimestampUs();
                        diag.last_sdk_seq = arm->lastCommandSeq();
                    }
                    diag.state_age_us = elapsedUs(now, state_time);
                    diag.stale_command_count = stale_ticks;
                }

                std::unique_lock<std::mutex> lock(wakeup_mutex);
                wakeup.wait_until(lock, next_tick, [this]
                {
                    return stop_requested.load(std::memory_order_acquire)
                           || finished.load(std::memory_order_acquire);
                });

                if (Clock::now() > next_tick + period)
                    next_tick = Clock::now();
            }

            arm->endControlSession();
        }

        Arm* arm{nullptr};
        uint64_t period_us{2000};
        uint64_t command_timeout_us{20000};
        std::thread thread;
        std::atomic<bool> stop_requested{false};
        std::atomic<bool> has_command{false};
        std::atomic<bool> finished{false};
        mutable std::mutex command_mutex;
        Torques latest_command{};
        Clock::time_point command_write_time{};
        mutable std::mutex state_mutex;
        ArmState latest_state{};
        Clock::time_point state_time{};
        mutable std::mutex diag_mutex;
        TorqueControlDiagnostics diag{};
        std::condition_variable wakeup;
        std::mutex wakeup_mutex;
    };

    ActiveControl<Torques>::ActiveControl(Arm& arm,
                                          double rate_hz,
                                          double command_timeout_ms,
                                          bool auto_start)
        : m_impl(std::make_shared<Impl>(arm, rate_hz, command_timeout_ms))
    {
        if (auto_start)
            m_impl->start();
    }

    ActiveControl<Torques>::~ActiveControl()
    {
        stop();
    }

    ActiveControl<Torques>::ActiveControl(ActiveControl&& other) noexcept
        : m_impl(std::move(other.m_impl))
    {
    }

    ActiveControl<Torques>& ActiveControl<Torques>::operator=(ActiveControl&& other) noexcept
    {
        if (this != &other)
        {
            stop();
            m_impl = std::move(other.m_impl);
        }
        return *this;
    }

    ArmState ActiveControl<Torques>::readOnce()
    {
        if (!m_impl) return ArmState{};
        std::lock_guard<std::mutex> lock(m_impl->state_mutex);
        return m_impl->latest_state;
    }

    void ActiveControl<Torques>::writeOnce(const Torques& cmd)
    {
        if (!m_impl || !m_impl->arm
            || m_impl->stop_requested.load(std::memory_order_acquire))
            return;

        std::lock_guard<std::mutex> lock(m_impl->command_mutex);
        m_impl->latest_command = cmd;
        m_impl->has_command.store(true, std::memory_order_release);
        m_impl->command_write_time = Impl::Clock::now();
        if (cmd.motion_finished)
            m_impl->finished.store(true, std::memory_order_release);
    }

    void ActiveControl<Torques>::stop()
    {
        if (m_impl)
            m_impl->stop();
    }

    TorqueControlDiagnostics ActiveControl<Torques>::diagnostics() const
    {
        if (!m_impl) return TorqueControlDiagnostics{};
        TorqueControlDiagnostics result{};
        {
            std::lock_guard<std::mutex> lock(m_impl->diag_mutex);
            result = m_impl->diag;
        }

        const auto now = Impl::Clock::now();
        {
            std::lock_guard<std::mutex> lock(m_impl->command_mutex);
            if (m_impl->has_command.load(std::memory_order_acquire))
                result.command_age_us = Impl::elapsedUs(now, m_impl->command_write_time);
        }
        {
            std::lock_guard<std::mutex> lock(m_impl->state_mutex);
            result.state_age_us = Impl::elapsedUs(now, m_impl->state_time);
        }
        return result;
    }

} // namespace florid
