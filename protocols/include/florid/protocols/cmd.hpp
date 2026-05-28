#ifndef FLORID_PROTOCOLS_CMD_HPP
#define FLORID_PROTOCOLS_CMD_HPP

#include "common.hpp"

namespace florid::protocol
{
    struct JointCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::JointCommand;
        PacketHeader header{.type = PacketType::JointCommand};
        uint64_t timestamp_us{};
        ControlMode control_mode{};
        CommandIntent intent{CommandIntent::PointTarget};
        uint8_t _pad[2]{};
        float kp[6]{};     // 每帧自带刚度和阻尼，消除 TCP/UDP 竞态
        float kd[6]{};
        JointState state;
    };

    struct CartesianPoseCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::CartesianPoseCommand;
        PacketHeader header{.type = PacketType::CartesianPoseCommand};
        uint64_t timestamp_us{};
        ControlMode control_mode{};
        CommandIntent intent{CommandIntent::PointTarget};
        uint8_t _pad[2]{};
        CartesianPose pose;
    };

    struct CartesianVelocityCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::CartesianVelocityCommand;
        PacketHeader header{.type = PacketType::CartesianVelocityCommand};
        uint64_t timestamp_us{};
        ControlMode control_mode{ControlMode::CartesianVelocity};
        CommandIntent intent{CommandIntent::PointTarget};
        uint8_t _pad[2]{};
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
        uint64_t timestamp_us{};
        ProtocolVersion version{};
        uint16_t watchdog_timeout_ms{500};
        uint16_t client_udp_port{0};  // arm 向此端口发 telemetry（0 = 不发送）
        SessionMode mode{};
        uint8_t _pad[1]{};
    };
}

#endif //FLORID_PROTOCOLS_CMD_HPP
