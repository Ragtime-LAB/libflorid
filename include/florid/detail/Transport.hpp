#ifndef FLORID_TRANSPORT_HPP
#define FLORID_TRANSPORT_HPP

#include <cstdint>
#include <cstddef>

namespace florid {

class Transport {
public:
    using ReceiveFunctor = void (*)(void* s_context, const std::uint8_t* s_data, std::size_t s_size);

    virtual ~Transport() = default;

    // Wirelink's single owner thread is the sole sender. Implementations do
    // not need to serialize concurrent calls to send().
    virtual bool send(const std::uint8_t* s_data, std::size_t s_size) = 0;

    // Passing nullptr is a quiescence barrier: when the call returns, no
    // previously installed callback is still running or can start running.
    // It must not be called recursively from inside the receive callback.
    virtual void setReceiveCallback(ReceiveFunctor s_callback, void* s_context) = 0;
};

} // namespace florid

#endif // FLORID_TRANSPORT_HPP
