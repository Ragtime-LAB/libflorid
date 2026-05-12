#ifndef FLORID_ARMCORE_HPP
#define FLORID_ARMCORE_HPP

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <florid/protocols/protocols.hpp>
#include <florid/detail/Seqlock.hpp>
#include <florid/detail/timestamp.hpp>
#include "traits.hpp"

namespace florid::core
{
    struct PackedCommandView
    {
        const uint8_t* data;
        size_t size;
    };

    class ArmCore
    {
    public:
        ArmCore();

        void feed_incoming_data(const uint8_t* data, size_t len);


        ArmStatus get_current_status();


        template <typename UserCommand>
        PackedCommandView pack_command(const UserCommand& cmd)
        {
            static_assert(
                is_control_command<UserCommand>::value,
                "Unsupported command type: expected florid::core::JointCommand or florid::core::CartesianPose");

            return pack_impl(cmd);
        }

    private:
        void handle_packet(const protocol::ArmStatusPacket& packet);

        template <typename T>
        void handle_packet(const T&)
        {
            // Ignored
        }

        PackedCommandView pack_impl(const JointCommand& cmd);
        PackedCommandView pack_impl(const CartesianPose& cmd);

        detail::SeqlockBuf<ArmStatus> m_latest_status;
        protocol::JointCmdPacket m_tx_joint_command{};
        protocol::CartesianPoseCmdPacket m_tx_cart_command{};
        uint32_t m_seq_num{};
    };
}

#endif //FLORID_ARMCORE_HPP
