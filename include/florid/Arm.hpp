#ifndef FLORID_ARM_HPP
#define FLORID_ARM_HPP

#include "core/ArmCore.hpp"
#include "core/Duration.hpp"
#include "core/ArmControl.hpp"
#include "detail/tick.hpp"
#include "detail/Transport.hpp"
#include <stddef.h>

namespace florid {

template <typename ControlType>
class ActiveControl;

class Arm {
public:
    // ── 嵌入式：手动注入 Transport ──────────────────────────────
    explicit Arm(Transport& control_transport,
                  Transport* config_transport = nullptr);

    // ── Host：自动创建 TCP+UDP ─────────────────────────────────
    Arm(const char* ip, uint16_t udp_port = 6040,
        protocol::SessionMode mode = protocol::SessionMode::Joint);

    Arm(const Arm&) = delete;
    Arm& operator=(const Arm&) = delete;
    Arm(Arm&&) = delete;
    Arm& operator=(Arm&&) = delete;
    ~Arm();

    // ── 状态读取 ───────────────────────────────────────────────

    ArmState readOnce();

    template <typename Callable>
    void read(Callable&& cb)
    {
        while (true)
        {
            update();
            auto state = m_core.get_arm_state();
            if (!cb(state)) break;
        }
    }

    // ── 控制 ───────────────────────────────────────────────────

    template <typename Callable>
    void control(Callable&& cb)
    {
        using ReturnType = decltype(cb(m_core.get_arm_state(),
                                       m_core.arm_control()));
        static_assert(
            is_control_command<ReturnType>::value,
            "Control callback must return Torques, JointPositions, "
            "JointVelocities, CartesianPose, or CartesianVelocities");

        auto& ctrl = m_core.arm_control();

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

            auto state = m_core.get_arm_state();
            auto cmd   = cb(state, m_core.arm_control());

            auto raw = m_core.pack_command(cmd);
            m_transport->send(raw.data, raw.size);

            if (cmd.motion_finished)
            {
                m_finish_flag = true;
            }
        }
    }

    template <typename TorqueCb, typename MotionCb>
    void control(TorqueCb&& torque_cb, MotionCb&& motion_cb)
    {
        auto& ctrl = m_core.arm_control();

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

            auto state      = m_core.get_arm_state();
            auto torque_cmd = torque_cb(state, ctrl);
            auto motion_cmd = motion_cb(state, ctrl);

            auto raw = m_core.pack_hybrid(torque_cmd, motion_cmd);
            m_transport->send(raw.data, raw.size);

            if (torque_cmd.motion_finished || motion_cmd.motion_finished)
            {
                m_finish_flag = true;
            }
        }
    }

    // ── 配置 ───────────────────────────────────────────────────

    void setMaxFrequencyHz(double hz);

    void setCollisionBehavior(const float (&lower_torques)[6],
                              const float (&upper_torques)[6]);

    void setCartesianImpedance(const float (&K_x)[6]);

    void setK(const float (&EE_T_K)[16]);

    void setEE(const float (&NE_T_EE)[16]);

    void setLoad(float mass,
                 const float (&F_x_Cload)[3],
                 const float (&load_inertia)[9]);

    void automaticErrorRecovery();

    void stop();

    void setWatchdogTimeout(Duration timeout);

    void switchControlMode(protocol::ControlMode mode);

    // ── 主动控制（高级用户手动循环） ────────────────────────────

    ActiveControl<Torques>              startTorqueControl();
    ActiveControl<JointPositions>       startJointPositionControl();
    ActiveControl<JointVelocities>      startJointVelocityControl();
    ActiveControl<CartesianPose>        startCartesianPoseControl();
    ActiveControl<CartesianVelocities>  startCartesianVelocityControl();

    ArmState update() { m_transport->poll(); return m_core.get_arm_state(); }

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

    ArmControl& controlHandle() { return m_core.arm_control(); }

private:
    static void on_receive_thunk(void* context, const uint8_t* data, size_t len);

    Transport*    m_transport{nullptr};
    Transport*    m_cfg_transport{nullptr};
    ArmCore       m_core;
    void*         m_host_impl{nullptr};  // PIMPL: owns TcpClient+UdpClient
    double        m_max_frequency_hz{0.0};
    double        m_next_tick_ms{0.0};
    bool          m_tick_initialized{false};
    bool          m_finish_flag{false};
};

} // namespace florid

#include "core/ActiveControl.hpp"

#endif // FLORID_ARM_HPP
