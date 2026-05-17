#ifndef FLORID_PROTOCOLS_DISPATCHER_HPP
#define FLORID_PROTOCOLS_DISPATCHER_HPP

#include <utility>
#include "cmd.hpp"
#include "status.hpp"
#include "config_cmd.hpp"

namespace florid::protocol
{
    template <typename... Packets>
    struct PacketRegistry
    {
    };

    using AllProtocolRegistry = PacketRegistry<
        JointCmdPacket,
        CartesianPoseCmdPacket,
        CartesianVelocityCmdPacket,
        ArmStatusPacket,
        SessionStatusPacket,
        ConfigCmdPacket
    >;

    using CmdProtocolRegistry = PacketRegistry<
        JointCmdPacket,
        CartesianPoseCmdPacket,
        CartesianVelocityCmdPacket
    >;

    using StatusProtocolRegistry = PacketRegistry<
        ArmStatusPacket,
        SessionStatusPacket
    >;

    template <typename Registry = AllProtocolRegistry>
    class Dispatcher
    {
    public:
        template <typename Handler>
        static bool dispatch(const uint8_t* data, std::size_t len, Handler&& handler)
        {
            if (len < sizeof(PacketHeader)) return false;

            const auto header = reinterpret_cast<const PacketHeader*>(data);

            if (header->magic_word != 0xA5) return false;

            return dispatch_impl(Registry{}, header, data, len, handler);
        }

    private:
        template <typename Handler>
        static bool dispatch_impl(PacketRegistry<>, const PacketHeader*, const uint8_t*, std::size_t, Handler&&)
        {
            return false;
        }

        template <typename Handler, typename T, typename... Rest>
        static bool dispatch_impl(PacketRegistry<T, Rest...>, const PacketHeader* header, const uint8_t* data,
                                  std::size_t len, Handler&& handler)
        {
            if (header->type == T::TYPE_ID)
            {
                if (header->length != len || len != sizeof(T)) return false;
                handler(*reinterpret_cast<const T*>(data));
                return true;
            }
            return dispatch_impl(PacketRegistry<Rest...>{}, header, data, len, std::forward<Handler>(handler));
        }
    };
}

#endif //FLORID_PROTOCOLS_DISPATCHER_HPP
