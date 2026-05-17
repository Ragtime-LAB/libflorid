#include <florid/core/ArmCore.hpp>
#include <cstring>

namespace florid::core
{
    using namespace florid;
    namespace
    {
        void copy_float(const float* src, float* dst, unsigned n) noexcept
        {
            for (unsigned i = 0; i < n; ++i) dst[i] = src[i];
        }

        void copy_float_reverse(const float* src, float* dst, unsigned n) noexcept
        {
            for (unsigned i = 0; i < n; ++i) dst[i] = src[i];
        }
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

    RobotState ArmCore::get_robot_state()
    {
        return m_latest_state.read();
    }

    void ArmCore::handle_packet(const protocol::ArmStatusPacket& packet)
    {
        m_latest_state.manipulate([&packet](RobotState& state)
        {
            state.time    = packet.timestamp;
            state.mode    = static_cast<RobotMode>(packet.mode);
            state.errors  = packet.errors;

            copy_float(packet.O_T_EE,       state.O_T_EE,       16);
            copy_float(packet.F_ext,        state.F_ext,        6);
            copy_float(packet.tau_desired,  state.tau_desired,  6);
            copy_float(packet.state.q,      state.q,            6);
            copy_float(packet.state.dq,     state.dq,           6);
            copy_float(packet.state.tau,    state.tau,          6);
        });
    }

    PackedCommandView ArmCore::pack_impl(const Torques& cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::Torque;
        copy_float(cmd.tau,              m_tx_joint_command.state.tau, 6);
        m_tx_joint_command.timestamp    = detail::get_timestamp();
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const JointPositions& cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::JointPosition;
        copy_float(cmd.q,               m_tx_joint_command.state.q, 6);
        m_tx_joint_command.timestamp    = detail::get_timestamp();
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const JointVelocities& cmd)
    {
        m_tx_joint_command.control_mode = protocol::ControlMode::JointVelocity;
        copy_float(cmd.dq,              m_tx_joint_command.state.dq, 6);
        m_tx_joint_command.timestamp    = detail::get_timestamp();
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const CartesianPose& cmd)
    {
        m_tx_cart_command.control_mode = protocol::ControlMode::CartesianPose;
        copy_float(cmd.T,              m_tx_cart_command.pose.T, 16);
        m_tx_cart_command.timestamp    = detail::get_timestamp();
        m_tx_cart_command.header.length = static_cast<uint16_t>(sizeof(m_tx_cart_command));
        m_tx_cart_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_cart_command), sizeof(m_tx_cart_command)};
    }

    PackedCommandView ArmCore::pack_impl(const CartesianVelocities& cmd)
    {
        m_tx_cart_vel_command.control_mode = protocol::ControlMode::CartesianVelocity;
        copy_float(cmd.v,                  m_tx_cart_vel_command.v, 6);
        m_tx_cart_vel_command.timestamp    = detail::get_timestamp();
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

} // namespace florid::core
