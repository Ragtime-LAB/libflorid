#ifndef FLORID_ACTIVE_CONTROL_HPP
#define FLORID_ACTIVE_CONTROL_HPP

#include "types.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace florid
{
    class Arm;

    template <typename ControlType>
    class ActiveControl
    {
    public:
        explicit ActiveControl(Arm& arm) : m_arm(&arm) {}

        ~ActiveControl() = default;

        ActiveControl(const ActiveControl&) = delete;
        ActiveControl& operator=(const ActiveControl&) = delete;

        ActiveControl(ActiveControl&& other) noexcept
            : m_arm(other.m_arm), m_finished(other.m_finished)
        {
            other.m_arm = nullptr;
        }

        ActiveControl& operator=(ActiveControl&& other) noexcept
        {
            if (this != &other)
            {
                m_arm      = other.m_arm;
                m_finished = other.m_finished;
                other.m_arm = nullptr;
            }
            return *this;
        }

        ArmState readOnce()
        {
            if (!m_arm) return ArmState{};
            return m_arm->update();
        }

        void writeOnce(const ControlType& cmd)
        {
            if (!m_arm) return;
            if (cmd.motion_finished) m_finished = true;
            m_arm->writeOnce(cmd);
        }

    private:
        Arm* m_arm;
        bool m_finished{false};
    };

    template <>
    class ActiveControl<Torques>
    {
    public:
        explicit ActiveControl(Arm& arm,
                               double rate_hz = 500.0,
                               double command_timeout_ms = 20.0,
                               bool auto_start = true);

        ~ActiveControl();

        ActiveControl(const ActiveControl&) = delete;
        ActiveControl& operator=(const ActiveControl&) = delete;

        ActiveControl(ActiveControl&& other) noexcept;
        ActiveControl& operator=(ActiveControl&& other) noexcept;

        ArmState readOnce();
        void writeOnce(const Torques& cmd);
        void stop();
        TorqueControlDiagnostics diagnostics() const;

        ActiveControl& enter()
        {
            return *this;
        }

        void exit()
        {
            stop();
        }

    private:
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };

} // namespace florid

#endif // FLORID_ACTIVE_CONTROL_HPP
