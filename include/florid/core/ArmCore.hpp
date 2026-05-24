#ifndef FLORID_ARMCORE_HPP
#define FLORID_ARMCORE_HPP

#include <cstdint>
#include <cstddef>
#include <florid/protocols/protocols.hpp>
#include <florid/detail/Seqlock.hpp>
#include <florid/detail/timestamp.hpp>
#include "traits.hpp"
#include "ArmControl.hpp"

namespace florid
{
    struct PackedCommandView
    {
        const uint8_t* data;
        size_t          size;
    };

    class ArmCore
    {
    public:
        ArmCore();

        void feed_incoming_data(const uint8_t* data, size_t len);

        ArmState get_arm_state();

        template <typename UserCommand>
        PackedCommandView pack_command(const UserCommand& cmd)
        {
            static_assert(
                is_control_command<UserCommand>::value,
                "Unsupported command type: expected Torques, JointPositions, "
                "JointVelocities, CartesianPose, or CartesianVelocities");

            return pack_impl(cmd);
        }

        PackedCommandView pack_config(protocol::ConfigType type,
                                      const float* data,
                                      unsigned count);

        PackedCommandView pack_hybrid(const Torques& torque_cmd,
                                      const JointPositions& motion_cmd);
        PackedCommandView pack_hybrid(const Torques& torque_cmd,
                                      const JointVelocities& motion_cmd);

        ArmControl& arm_control() { return m_arm_control; }

        void set_gains(const float* kp, const float* kd)
        {
            for (int i = 0; i < 6; ++i) { m_kp[i] = kp[i]; m_kd[i] = kd[i]; }
        }

    private:
        void handle_packet(const protocol::ArmStatusPacket& packet);

        template <typename T>
        void handle_packet(const T&) {}

        PackedCommandView pack_impl(const Torques& cmd);
        PackedCommandView pack_impl(const JointPositions& cmd);
        PackedCommandView pack_impl(const JointVelocities& cmd);
        PackedCommandView pack_impl(const CartesianPose& cmd);
        PackedCommandView pack_impl(const CartesianVelocities& cmd);

        detail::SeqlockBuf<ArmState>    m_latest_state;
        protocol::JointCmdPacket          m_tx_joint_command{};
        protocol::CartesianPoseCmdPacket  m_tx_cart_command{};
        protocol::CartesianVelocityCmdPacket m_tx_cart_vel_command{};
        protocol::ConfigCmdPacket         m_tx_config_command{};
        uint32_t                          m_seq_num{};
        bool                              m_finish_flag{false};
        bool                              m_stop_flag{false};
        float                             m_kp[6]{};
        float                             m_kd[6]{5, 5, 5, 5, 5, 5};
        ArmControl                      m_arm_control{&m_stop_flag};
    };
}

#endif //FLORID_ARMCORE_HPP
