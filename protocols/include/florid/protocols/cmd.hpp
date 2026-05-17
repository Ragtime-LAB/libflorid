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
        ControlMode control_mode{};
        uint8_t _pad[3]{};
        JointState state;
    };

    struct CartesianPoseCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::CartesianPoseCommand;
        PacketHeader header{.type = PacketType::CartesianPoseCommand};
        double timestamp{};
        ControlMode control_mode{};
        uint8_t _pad[3]{};
        CartesianPose pose;
    };

    struct CartesianVelocityCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::CartesianVelocityCommand;
        PacketHeader header{.type = PacketType::CartesianVelocityCommand};
        double timestamp{};
        ControlMode control_mode{ControlMode::CartesianVelocity};
        uint8_t _pad[3]{};
        float v[6]{};  // {vx, vy, vz, wx, wy, wz}
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
        uint16_t watchdog_timeout_ms{500};
        SessionMode mode{};
        uint8_t _pad[3]{};
    };
}

#endif //FLORID_PROTOCOLS_CMD_HPP
