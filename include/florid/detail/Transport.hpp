#ifndef FLORID_TRANSPORT_HPP
#define FLORID_TRANSPORT_HPP

#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>

namespace florid {

// Control traffic can discard stale commands; reliable traffic cannot.
enum class TxClass : std::uint8_t {
    ControlLatest,
    Reliable,
};

// Result of submitting a frame to the transport. A queued frame reports its
// eventual I/O result through TxCompletion.
enum class TxSubmitResult : std::uint8_t {
    Accepted,
    Disconnected,
    QueueFull,
    IoError,
};

enum class TxCompletionStatus : std::uint8_t {
    Completed,
    Disconnected,
    Cancelled,
    IoError,
};

class Transport {
public:
    using ReceiveFunctor = void (*)(void* s_context, const std::uint8_t* s_data, std::size_t s_size);
    using TxCompletion = std::function<void(TxCompletionStatus)>;

    virtual ~Transport() = default;

    virtual TxSubmitResult submit(TxClass s_class,
                                  std::span<const std::uint8_t> s_data,
                                  TxCompletion s_completion = {}) = 0;

    virtual void setReceiveCallback(ReceiveFunctor s_callback, void* s_context) = 0;

    virtual void poll() = 0;
};

} // namespace florid

#endif // FLORID_TRANSPORT_HPP
