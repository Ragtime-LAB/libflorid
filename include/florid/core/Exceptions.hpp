#ifndef FLORID_EXCEPTIONS_HPP
#define FLORID_EXCEPTIONS_HPP

namespace florid
{
    struct Exception
    {
        const char* what;
    };

    struct NetworkException : Exception {};

    struct ProtocolException : Exception {};

    struct IncompatibleVersionException : Exception {};

    struct ControlException : Exception
    {
        uint32_t lost_packets;
        uint32_t late_packets;
    };

    struct RealtimeException : Exception {};

    struct InvalidOperationException : Exception {};

    struct CommandException : Exception {};

} // namespace florid

#endif // FLORID_EXCEPTIONS_HPP
