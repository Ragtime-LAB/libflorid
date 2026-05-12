#ifndef FLORID_PROTOCOLS_STATUS_HPP
#define FLORID_PROTOCOLS_STATUS_HPP

#include "common.hpp"

namespace florid::protocol
{
    enum class SessionStatus: uint8_t
    {
        SUCCESS = 0,
        FAILURE = 1,
    };

    enum class ArmMode : uint8_t
    {
        INIT = 0,
        IDLE = 1,
        RUNNING = 2,
        FAULT = 3,
        ESTOP = 4,
    };

    struct SessionStatusPacket
    {
        static constexpr auto TYPE_ID = PacketType::SessionStatus;
        PacketHeader header{.type = PacketType::SessionStatus};
        ProtocolVersion version{};
        SessionStatus status{};
        uint8_t _pad[5]{};
    };

    struct ArmStatusPacket
    {
        static constexpr auto TYPE_ID = PacketType::ArmState;
        PacketHeader header{.type = PacketType::ArmState};
        ArmMode mode{};
        uint8_t _pad[7]{};
        JointState state;
    };
}

#endif //FLORID_PROTOCOLS_STATUS_HPP
