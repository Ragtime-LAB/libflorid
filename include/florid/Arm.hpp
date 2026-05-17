#ifndef FLORID_ARM_HPP
#define FLORID_ARM_HPP

#include "core/ArmCore.hpp"
#include "core/Duration.hpp"
#include "core/RobotControl.hpp"
#include "detail/tick.hpp"
#include "detail/Transport.hpp"
#include <stddef.h>

namespace florid {

class Arm {
public:
    explicit Arm(Transport& control_transport,
                  Transport* config_transport = nullptr);

    Arm(const Arm&) = delete;
    Arm& operator=(const Arm&) = delete;
    Arm(Arm&&) = delete;
    Arm& operator=(Arm&&) = delete;
    ~Arm() = default;

    // ── 状态读取 ───────────────────────────────────────────────

    RobotState readOnce();

    template <typename Callable>
    void read(Callable&& cb)
    {
        while (true)
        {
            update();
            auto state = m_core.get_robot_state();
            if (!cb(state)) break;
        }
    }

    // ── 控制 ───────────────────────────────────────────────────

    template <typename Callable>
    void control(Callable&& cb)
    {
        using ReturnType = decltype(cb(m_core.get_robot_state(),
                                       m_core.robot_control()));
        static_assert(
            is_control_command<ReturnType>::value,
            "Control callback must return Torques, JointPositions, "
            "JointVelocities, CartesianPose, or CartesianVelocities");

        auto& ctrl = m_core.robot_control();

        m_finish_flag = false;
        ctrl.setFinishFlag(&m_finish_flag);

        while (!ctrl.isStopped() && !m_finish_flag)
        {
            update();

            if (m_max_frequency_hz > 0.0)
            {
                const double now = detail::get_tick_ms();
                const double period = 1000.0 / m_max_frequency_hz;

                if (!m_tick_initialized)
                {
                    m_next_tick_ms     = now;
                    m_tick_initialized = true;
                }

                if (now < m_next_tick_ms) continue;

                m_next_tick_ms += period;
                if (m_next_tick_ms <= now)
                    m_next_tick_ms = now + period;
            }

            auto state = m_core.get_robot_state();
            auto cmd   = cb(state, m_core.robot_control());

            auto raw = m_core.pack_command(cmd);
            m_transport->send(raw.data, raw.size);

            if (cmd.motion_finished)
            {
                m_finish_flag = true;
            }
        }
    }

    // ── 配置 ───────────────────────────────────────────────────

    void setMaxFrequencyHz(double hz);

    void setCollisionBehavior(const float (&lower_torques)[6],
                              const float (&upper_torques)[6]);

    void setJointImpedance(const float (&K_theta)[6]);

    void setCartesianImpedance(const float (&K_x)[6]);

    void setK(const float (&EE_T_K)[16]);

    void setEE(const float (&NE_T_EE)[16]);

    void setLoad(float mass,
                 const float (&F_x_Cload)[3],
                 const float (&load_inertia)[9]);

    void automaticErrorRecovery();

    void stop();

    void setWatchdogTimeout(Duration timeout);

    // ── 主动控制（高级用户手动循环） ────────────────────────────

    RobotState update() { m_transport->poll(); return m_core.get_robot_state(); }

    template <typename Command>
    void writeOnce(const Command& cmd)
    {
        static_assert(
            is_control_command<Command>::value,
            "writeOnce requires Torques, JointPositions, "
            "JointVelocities, CartesianPose, or CartesianVelocities");

        auto raw = m_core.pack_command(cmd);
        m_transport->send(raw.data, raw.size);
    }

    RobotControl& controlHandle() { return m_core.robot_control(); }

private:
    static void on_receive_thunk(void* context, const uint8_t* data, size_t len);

    Transport*    m_transport;
    Transport*    m_cfg_transport;
    core::ArmCore m_core;
    double        m_max_frequency_hz{0.0};
    double        m_next_tick_ms{0.0};
    bool          m_tick_initialized{false};
    bool          m_finish_flag{false};
};

} // namespace florid

#endif // FLORID_ARM_HPP
