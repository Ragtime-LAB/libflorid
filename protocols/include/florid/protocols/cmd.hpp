#ifndef FLORID_PROTOCOLS_CMD_HPP
#define FLORID_PROTOCOLS_CMD_HPP

#include "common.hpp"

namespace florid::protocol
{
    struct JointCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::JointCommand;
        PacketHeader header{.type = PacketType::JointCommand};
        double timestamp{};
        JointState state;
    };

    struct CartesianPoseCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::CartesianPoseCommand;
        PacketHeader header{.type = PacketType::CartesianPoseCommand};
        double timestamp{};
        CartesianPose pose;
    };

    enum class SessionMode : uint8_t
    {
        Joint,
        Point,
        Stop,
    };

    struct SessionCfgPacket
    {
        static constexpr auto TYPE_ID = PacketType::SessionConfig;
        PacketHeader header{.type = PacketType::SessionConfig};
        double timestamp{};
        ProtocolVersion version{};
        SessionMode mode{};
        uint8_t _pad[5]{};
    };
}

#endif //FLORID_PROTOCOLS_CMD_HPP
