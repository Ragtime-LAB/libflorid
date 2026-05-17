#ifndef FLORID_PROTOCOLS_COMMON_HPP
#define FLORID_PROTOCOLS_COMMON_HPP

#include <cstdint>

namespace florid::protocol
{
    enum class PacketType : uint8_t
    {
        JointCommand = 0x11,
        CartesianPoseCommand = 0x12,
        ArmState = 0x21,
        SessionConfig = 0x31,
        SessionStatus = 0x32,
        CartesianVelocityCommand = 0x13,
        ConfigCommand = 0x41
    };

    enum class ControlMode : uint8_t
    {
        JointPosition = 0x01,
        JointVelocity = 0x02,
        Torque = 0x03,
        CartesianPose = 0x04,
        CartesianVelocity = 0x05
    };

    struct PacketHeader
    {
        uint8_t magic_word = 0xA5;
        PacketType type{};
        uint16_t length{};
        uint32_t seq_num{};
    };

    struct JointState
    {
        float q[6];   ///> 位置
        float dq[6];  ///> 速度
        float tau[6]; ///> 扭矩
    };

    struct CartesianPose
    {
        float T[16]; ///> 4x4 齐次变换矩阵（列优先）
    };

    using ProtocolVersion = uint16_t;
}

#endif //FLORID_PROTOCOLS_COMMON_HPP
