#ifndef FLORID_ACTIVE_CONTROL_HPP
#define FLORID_ACTIVE_CONTROL_HPP

#include "types.hpp"

namespace florid
{
    class Arm;

    template <typename ControlType>
    class ActiveControl
    {
    public:
        explicit ActiveControl(Arm& arm) : m_arm(&arm) {}

        ~ActiveControl()
        {
            if (m_arm && !m_finished)
            {
                ControlType cmd{};
                cmd.motion_finished = true;
                m_arm->writeOnce(cmd);
            }
        }

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

        RobotState readOnce()
        {
            if (!m_arm) return RobotState{};
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

} // namespace florid

#endif // FLORID_ACTIVE_CONTROL_HPP
