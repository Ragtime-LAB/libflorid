#include <florid/core/ArmCore.hpp>
#include <cstring>

namespace florid
{
    namespace
    {
        void copy_float(const float* src, float* dst, unsigned n) noexcept
        {
            for (unsigned i = 0; i < n; ++i) dst[i] = src[i];
        }

        // 非零则用 cmd_gains，全零则回退 stored_gains
        void merge_gains(const float* cmd_kp, const float* cmd_kd,
                         const float* stored_kp, const float* stored_kd,
                         float* out_kp, float* out_kd)
        {
            bool has_kp = false, has_kd = false;
            for (int i = 0; i < 6; ++i)
            {
                if (cmd_kp[i] != 0.0f) has_kp = true;
                if (cmd_kd[i] != 0.0f) has_kd = true;
                out_kp[i] = has_kp ? cmd_kp[i] : stored_kp[i];
                out_kd[i] = has_kd ? cmd_kd[i] : stored_kd[i];
            }
        }
    }

    uint64_t command_timestamp_us(bool active, double session_start_timestamp)
    {
        if (!active)
            return 0;
        return static_cast<uint64_t>((detail::get_timestamp() - session_start_timestamp) * 1000.0);
    }

    protocol::CommandIntent command_intent(bool active)
    {
        return active ? protocol::CommandIntent::Stream
                      : protocol::CommandIntent::PointTarget;
    }

    ArmCore::ArmCore()
    {
        m_tx_config_command.header.length = static_cast<uint16_t>(sizeof(m_tx_config_command));
    }

    void ArmCore::feed_incoming_data(const uint8_t* data, const size_t len)
    {
        protocol::Dispatcher<protocol::StatusProtocolRegistry>::dispatch(
            data, len,
            [this](const auto& packet) { this->handle_packet(packet); });
    }

    ArmState ArmCore::get_arm_state()
    {
        return m_latest_state.read();
    }

    void ArmCore::begin_control_session()
    {
        m_control_session_start_timestamp = detail::get_timestamp();
        m_control_session_active = true;
    }

    void ArmCore::end_control_session()
    {
        m_control_session_active = false;
    }

    void ArmCore::handle_packet(const protocol::ArmStatusPacket& packet)
    {
        m_latest_state.manipulate([&packet](ArmState& state)
        {
            state.time    = packet.timestamp;
            state.mode    = static_cast<ArmMode>(packet.mode);
            state.errors  = packet.errors;

            copy_float(packet.O_T_EE,       state.O_T_EE,       16);
            copy_float(packet.F_ext,        state.F_ext,        6);
            copy_float(packet.base_gravity, state.base_gravity, 3);
            copy_float(packet.state.q,      state.q,            6);
            copy_float(packet.state.dq,     state.dq,           6);
            copy_float(packet.state.tau,    state.tau,          6);
        });
    }

    PackedCommandView ArmCore::pack_impl(const Torques& cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::Torque;
        for (int i = 0; i < 6; ++i) {
            m_tx_joint_command.state.q[i]  = 0;
            m_tx_joint_command.state.dq[i] = 0;
        }
        copy_float(cmd.tau,              m_tx_joint_command.state.tau, 6);
        merge_gains(cmd.kp, cmd.kd, m_kp, m_kd,
                    m_tx_joint_command.kp, m_tx_joint_command.kd);
        m_tx_joint_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_joint_command.intent = command_intent(m_control_session_active);
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const JointPositions& cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::JointPosition;
        for (int i = 0; i < 6; ++i) {
            m_tx_joint_command.state.dq[i]  = cmd.dq[i];
            m_tx_joint_command.state.tau[i] = 0;
        }
        copy_float(cmd.q,               m_tx_joint_command.state.q, 6);
        copy_float(cmd.dq,              m_tx_joint_command.state.dq, 6);
        merge_gains(cmd.kp, cmd.kd, m_kp, m_kd,
                    m_tx_joint_command.kp, m_tx_joint_command.kd);
        m_tx_joint_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_joint_command.intent = command_intent(m_control_session_active);
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const JointVelocities& cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::JointVelocity;
        for (int i = 0; i < 6; ++i) {
            m_tx_joint_command.state.q[i]   = 0;
            m_tx_joint_command.state.tau[i] = 0;
        }
        copy_float(cmd.dq,              m_tx_joint_command.state.dq, 6);
        merge_gains(cmd.kp, cmd.kd, m_kp, m_kd,
                    m_tx_joint_command.kp, m_tx_joint_command.kd);
        m_tx_joint_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_joint_command.intent = command_intent(m_control_session_active);
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const CartesianPose& cmd)
    {
        m_tx_cart_command.control_mode = protocol::ControlMode::CartesianPose;
        copy_float(cmd.T,              m_tx_cart_command.pose.T, 16);
        merge_gains(cmd.kp, cmd.kd, m_kp, m_kd,
                    m_tx_cart_command.kp, m_tx_cart_command.kd);
        m_tx_cart_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_cart_command.intent = command_intent(m_control_session_active);
        m_tx_cart_command.header.length = static_cast<uint16_t>(sizeof(m_tx_cart_command));
        m_tx_cart_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_cart_command), sizeof(m_tx_cart_command)};
    }

    PackedCommandView ArmCore::pack_impl(const CartesianVelocities& cmd)
    {
        m_tx_cart_vel_command.control_mode = protocol::ControlMode::CartesianVelocity;
        copy_float(cmd.v,                  m_tx_cart_vel_command.v, 6);
        merge_gains(cmd.kp, cmd.kd, m_kp, m_kd,
                    m_tx_cart_vel_command.kp, m_tx_cart_vel_command.kd);
        m_tx_cart_vel_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_cart_vel_command.intent = command_intent(m_control_session_active);
        m_tx_cart_vel_command.header.length = static_cast<uint16_t>(sizeof(m_tx_cart_vel_command));
        m_tx_cart_vel_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_cart_vel_command), sizeof(m_tx_cart_vel_command)};
    }

    PackedCommandView ArmCore::pack_config(protocol::ConfigType type,
                                           const float* data,
                                           unsigned count)
    {
        m_tx_config_command.config_type = type;
        m_tx_config_command.timestamp   = detail::get_timestamp();
        m_tx_config_command.header.seq_num = ++m_seq_num;

        for (unsigned i = 0; i < count && i < 16; ++i)
            m_tx_config_command.data[i] = data[i];
        for (unsigned i = count; i < 16; ++i)
            m_tx_config_command.data[i] = 0.0f;

        return {reinterpret_cast<const uint8_t*>(&m_tx_config_command), sizeof(m_tx_config_command)};
    }

    PackedCommandView ArmCore::pack_hybrid(const Torques& torque_cmd,
                                           const JointPositions& motion_cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::JointPosition;
        copy_float(motion_cmd.q,        m_tx_joint_command.state.q,   6);
        copy_float(motion_cmd.dq,       m_tx_joint_command.state.dq,  6);
        copy_float(torque_cmd.tau,      m_tx_joint_command.state.tau, 6);
        copy_float(m_kp,                m_tx_joint_command.kp, 6);
        copy_float(m_kd,                m_tx_joint_command.kd, 6);
        m_tx_joint_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_joint_command.intent = command_intent(m_control_session_active);
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_hybrid(const Torques& torque_cmd,
                                           const JointVelocities& motion_cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::JointVelocity;
        copy_float(motion_cmd.dq,       m_tx_joint_command.state.dq,  6);
        copy_float(torque_cmd.tau,      m_tx_joint_command.state.tau, 6);
        copy_float(m_kp,                m_tx_joint_command.kp, 6);
        copy_float(m_kd,                m_tx_joint_command.kd, 6);
        m_tx_joint_command.timestamp_us = command_timestamp_us(m_control_session_active, m_control_session_start_timestamp);
        m_tx_joint_command.intent = command_intent(m_control_session_active);
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

} // namespace florid
