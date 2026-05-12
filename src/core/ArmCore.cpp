#include <florid/core/ArmCore.hpp>

namespace florid::core
{
    ArmCore::ArmCore() = default;

    void ArmCore::feed_incoming_data(const uint8_t* data, const size_t len)
    {
        protocol::Dispatcher<protocol::StatusProtocolRegistry>::dispatch(data, len, [this](const auto& packet)
        {
            this->handle_packet(packet);
        });
    }

    ArmStatus ArmCore::get_current_status()
    {
        return m_latest_status.read();
    }

    void ArmCore::handle_packet(const protocol::ArmStatusPacket& packet)
    {
        m_latest_status.manipulate([&packet](ArmStatus& status)
        {
            status.state = packet.state;
            status.mode = packet.mode;
        });
    }

    PackedCommandView ArmCore::pack_impl(const JointCommand& cmd)
    {
        m_tx_joint_command.state = cmd;
        m_tx_joint_command.timestamp = detail::get_timestamp();
        m_tx_joint_command.header.length = static_cast<uint16_t>(sizeof(m_tx_joint_command));
        m_tx_joint_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_joint_command), sizeof(m_tx_joint_command)};
    }

    PackedCommandView ArmCore::pack_impl(const CartesianPose& cmd)
    {
        // 齐次变换矩阵只需拷贝前 12 个元素（3×4 旋转+平移），
        // 最后一行 [0 0 0 1] 由 m_tx_cart_command 的零初始化保证。
        std::copy_n(cmd.T, 12, m_tx_cart_command.pose.T);
        m_tx_cart_command.timestamp = detail::get_timestamp();
        m_tx_cart_command.header.length = static_cast<uint16_t>(sizeof(m_tx_cart_command));
        m_tx_cart_command.header.seq_num = ++m_seq_num;
        return {reinterpret_cast<const uint8_t*>(&m_tx_cart_command), sizeof(m_tx_cart_command)};
    }
}
