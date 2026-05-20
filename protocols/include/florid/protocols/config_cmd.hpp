#ifndef FLORID_PROTOCOLS_CONFIG_CMD_HPP
#define FLORID_PROTOCOLS_CONFIG_CMD_HPP

#include "common.hpp"

namespace florid::protocol
{
    enum class ConfigType : uint8_t
    {
        Stop                    = 0x01,
        AutomaticErrorRecovery  = 0x02,
        SetCollisionBehavior    = 0x10,
        SetJointImpedance       = 0x11,
        SetCartesianImpedance   = 0x12,
        SetK                    = 0x13,
        SetEE                   = 0x14,
        SetLoad                 = 0x15,
        SetWatchdogTimeout      = 0x16,
        SwitchControlMode       = 0x20,
    };

    struct ConfigCmdPacket
    {
        static constexpr auto TYPE_ID = PacketType::ConfigCommand;
        PacketHeader header{.type = PacketType::ConfigCommand};
        ConfigType config_type{};
        uint8_t _pad0[3]{};
        double timestamp{};
        float data[16]{};
    };
}

#endif //FLORID_PROTOCOLS_CONFIG_CMD_HPP
