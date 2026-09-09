#include "florid/detail/LatestValue.hpp"
#include "florid/detail/ControlGeneration.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <semaphore>
#include <stdexcept>
#include <thread>

namespace {
using namespace florid::detail;
using namespace std::chrono_literals;
void require(bool s_ok, const char* s_message) { if (!s_ok) throw std::runtime_error(s_message); }
struct alignas(64) Value {
    std::uint64_t m_sequence{};
    std::array<std::uint64_t, 127> m_payload{};
};
Value value(std::uint64_t s_sequence) {
    Value s_value;
    s_value.m_sequence = s_sequence;
    for (std::size_t i = 0; i < s_value.m_payload.size(); ++i) s_value.m_payload[i] = s_sequence ^ i;
    return s_value;
}
void testLatest() {
    LatestValue<Value> s_mailbox;
    auto s_cache = value(7);
    require(s_mailbox.read(s_cache) == WL_ERR_NO_DATA && s_cache.m_sequence == 7,
            "empty mailbox changed cache");
    require(s_mailbox.publish(value(1)) == WL_OK && s_mailbox.publish(value(2)) == WL_OK,
            "publication failed");
    require(s_mailbox.read(s_cache) == WL_OK && s_cache.m_sequence == 2, "did not coalesce to latest");
    require(s_mailbox.read(s_cache) == WL_ERR_NO_DATA && s_cache.m_sequence == 2, "replayed publication");
    constexpr std::uint64_t s_last = 100'000;
    std::atomic<bool> s_ok{true};
    std::jthread s_writer([&] {
        for (std::uint64_t i = 3; i <= s_last; ++i)
            if (s_mailbox.publish(value(i)) != WL_OK) s_ok = false;
    });
    const auto s_deadline = std::chrono::steady_clock::now() + 5s;
    while (s_cache.m_sequence != s_last) {
        const auto s_previous = s_cache.m_sequence;
        const auto s_read = s_mailbox.read(s_cache);
        require(s_read == WL_OK || s_read == WL_ERR_NO_DATA, "concurrent read failed");
        require(s_cache.m_sequence >= s_previous, "mailbox went backwards");
        for (std::size_t i = 0; i < s_cache.m_payload.size(); ++i)
            require(s_cache.m_payload[i] == (s_cache.m_sequence ^ i), "torn mailbox snapshot");
        require(std::chrono::steady_clock::now() < s_deadline, "lost last publication");
    }
    s_writer.join();
    require(s_ok, "concurrent publish failed");
}
void testGeneration() {
    ControlGeneration s_gate;
    const auto s_old = s_gate.current();
    std::binary_semaphore s_entered{0}, s_release{0};
    std::atomic<bool> s_drained{false};
    std::jthread s_writer([&] {
        s_gate.send(s_old, [&] { s_entered.release(); s_release.acquire(); });
    });
    s_entered.acquire();
    std::jthread s_switch([&] { s_gate.invalidate(); s_drained = true; });
    while (s_gate.current() == s_old) std::this_thread::yield();
    const bool s_waited = !s_drained.load();
    bool s_rejected = false;
    try { s_gate.send(s_old, [] {}); } catch (const florid::ControlException&) { s_rejected = true; }
    s_release.release();
    s_writer.join(); s_switch.join();
    require(s_waited && s_rejected && s_drained, "mode change did not drain/reject old submissions");
    bool s_sent = false;
    s_gate.send(s_gate.current(), [&] { s_sent = true; });
    require(s_sent, "new generation rejected");
    try { s_gate.send(s_gate.current(), [] { throw std::runtime_error("send failed"); }); }
    catch (const std::runtime_error&) {}
    s_gate.invalidate(); // A throwing sender must also leave the in-flight count.
    s_rejected = false;
    try { s_gate.send(s_old, [] {}); } catch (const florid::ControlException&) { s_rejected = true; }
    require(s_rejected, "old handle revived");

    std::atomic<unsigned> s_submitted{0};
    std::jthread s_racer([&](std::stop_token s_stop) {
        while (!s_stop.stop_requested()) {
            try { s_gate.send(s_gate.current(), [&] { ++s_submitted; }); }
            catch (const florid::ControlException&) {}
        }
    });
    while (!s_submitted) std::this_thread::yield();
    for (int i = 0; i < 1000; ++i) s_gate.invalidate();
    s_racer.request_stop(); s_racer.join();
    s_gate.invalidate();
}
}
int main() {
    try { testLatest(); testGeneration(); }
    catch (const std::exception& s_error) {
        std::fprintf(stderr, "test_control_exchange: %s\n", s_error.what()); return 1;
    }
}
