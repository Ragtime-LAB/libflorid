#ifndef FLORID_TRANSPORT_HPP
#define FLORID_TRANSPORT_HPP

#include <cstdint>
#include <cstddef>

#include <wirelink/wirelink.h>

namespace florid {

class Transport {
public:
    using ReceiveFunctor = void (*)(void* s_context, const std::uint8_t* s_data, std::size_t s_size);
    using WakeFunctor = void (*)(void* s_context) noexcept;

    virtual ~Transport() = default;

    // Wirelink's single owner thread is the sole sender. Implementations do
    // not need to serialize concurrent calls to send().
    virtual bool send(const std::uint8_t* s_data, std::size_t s_size) = 0;

    // Passing nullptr is a quiescence barrier: when the call returns, no
    // previously installed callback is still running or can start running.
    // It must not be called recursively from inside the receive callback.
    virtual void setReceiveCallback(ReceiveFunctor s_callback, void* s_context) = 0;

    // A direct transport owns Wirelink's physical RX claims and sink. This is
    // setup-only and lets Wirelink adapters receive directly into core-owned
    // storage. All service calls still run on the endpoint's owner thread.
    virtual bool usesDirectWirelink() const noexcept { return false; }
    virtual int attachWirelink(wl_ctx_t&, WakeFunctor, void*) noexcept {
        return WL_ERR_NOT_SUPPORTED;
    }
    virtual int serviceWirelink() noexcept { return WL_ERR_NOT_SUPPORTED; }
    virtual void quiesceWirelink() noexcept {}
    virtual std::uint32_t wirelinkDeadlineHint(
        wl_time_ms_t) const noexcept { return WL_POLL_NO_DEADLINE_MS; }
};

} // namespace florid

#endif // FLORID_TRANSPORT_HPP
