#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/Seqlock.hpp"

#include "fci_protocol/arm/packets.hpp"
#include "fci_protocol/session/arm_control_session.hpp"
#include "fci_protocol/transport/byte_stream_transport.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

using namespace florid;

// ────────────────────────────────────────────────────────────
//  MockTransport: captures sent bytes, can inject received bytes
// ────────────────────────────────────────────────────────────

class MockTransport : public Transport {
public:
    bool send(const std::uint8_t* s_data, std::size_t s_size) override {
        m_sent.insert(m_sent.end(), s_data, s_data + s_size);
        return true;
    }

    void setReceiveCallback(ReceiveFunctor s_callback, void* s_context) override {
        m_recv_cb = s_callback;
        m_recv_ctx = s_context;
    }

    void poll() override {}

    // Inject bytes into ArmImpl's receive pipeline
    void inject(const std::vector<uint8_t>& s_bytes) {
        if (m_recv_cb) {
            m_recv_cb(m_recv_ctx, s_bytes.data(), s_bytes.size());
        }
    }

    std::vector<uint8_t> m_sent;
    ReceiveFunctor m_recv_cb{nullptr};
    void* m_recv_ctx{nullptr};
};

// ────────────────────────────────────────────────────────────
//  Helper: serialize a known-good ArmStatus frame
// ────────────────────────────────────────────────────────────

struct DummyTick {
    using tick_type = std::uint32_t;
    static tick_type now() { return 1; }
};

std::vector<std::uint8_t> serializeArmStatus(fci::arm::ArmStatus& s_status) {
    using Session = RPL::USBTransport<
        RPL::AckManager<DummyTick>,
        std::function<void(const std::uint8_t*, std::size_t)>,
        USBAck,
        fci::arm::ArmStatus>;

    std::vector<uint8_t> s_bytes;
    Session s_sess;
    s_sess.on_send([&s_bytes](const uint8_t* d, size_t n) {
        s_bytes.assign(d, d + n);
    });

    auto s_result = s_sess.notify(s_status);
    assert(s_result.has_value());
    return s_bytes;
}

// ────────────────────────────────────────────────────────────
//  Tests
// ────────────────────────────────────────────────────────────

void test_arm_status_roundtrip() {
    printf("Test 1: ArmStatus round-trip through MockTransport\n");

    auto s_transport = std::make_unique<MockTransport>();
    auto* s_mock = static_cast<MockTransport*>(s_transport.get());

    // Create ArmImpl — this will try to fetch DeviceInfo but
    // there's no firmware to respond. DeviceInfo defaults will be used.
    ArmImpl s_impl(std::move(s_transport));

    // Verify defaults
    assert(s_impl.firmwarePeriodUs() == 2000);

    // Build a fake ArmStatus
    fci::arm::ArmStatus s_status{};
    s_status.seq = 123;
    s_status.timestamp_us = 999000;
    s_status.mode = fci::arm::ArmMode::Running;
    s_status.errors = 0x0A;
    s_status.status.q[0] = 1.0f;
    s_status.status.q[1] = 2.0f;
    s_status.status.q[2] = 3.0f;
    s_status.status.q[3] = 4.0f;
    s_status.status.q[4] = 5.0f;
    s_status.status.q[5] = 6.0f;
    s_status.status.dq[0] = 0.1f;
    s_status.base_gravity[0] = 0.0f;
    s_status.base_gravity[1] = 0.0f;
    s_status.base_gravity[2] = -9.81f;
    s_status.O_T_EE[15] = 1.0f;
    s_status.F_ext[0] = 5.5f;
    s_status.gripper.q = 0.05f;

    // Serialize the ArmStatus to wire bytes
    auto s_frame = serializeArmStatus(s_status);
    printf("  Frame size: %zu bytes\n", s_frame.size());

    // Inject the bytes into ArmImpl via MockTransport
    s_mock->inject(s_frame);

    // Read from ArmImpl
    auto s_state = s_impl.readOnce();
    printf("  seq=%u q0=%f q1=%f err=0x%02X gz=%f\n",
           s_state.m_seq, s_state.m_q[0], s_state.m_q[1],
           s_state.m_errors, s_state.m_base_gravity[2]);

    assert(s_state.m_seq == 123);
    assert(s_state.m_q[0] == 1.0f);
    assert(s_state.m_q[2] == 3.0f);
    assert(s_state.m_dq[0] == 0.1f);
    assert(s_state.m_mode == ArmMode::Running);
    assert(s_state.m_errors == 0x0A);
    assert(s_state.m_base_gravity[2] == -9.81f);
    assert(s_state.m_O_T_EE[15] == 1.0f);
    assert(s_state.m_F_ext[0] == 5.5f);
    assert(s_state.m_gripper_q == 0.05f);

    printf("  PASS\n");
}

void test_multiple_frames() {
    printf("Test 2: Multiple sequential ArmStatus frames\n");

    auto s_transport = std::make_unique<MockTransport>();
    auto* s_mock = static_cast<MockTransport*>(s_transport.get());
    ArmImpl s_impl(std::move(s_transport));

    for (uint32_t s_i = 0; s_i < 10; ++s_i) {
        fci::arm::ArmStatus s_status{};
        s_status.seq = s_i;
        s_status.status.q[0] = static_cast<float>(s_i);

        auto s_frame = serializeArmStatus(s_status);
        s_mock->inject(s_frame);

        auto s_state = s_impl.readOnce();
        assert(s_state.m_seq == s_i);
        assert(s_state.m_q[0] == static_cast<float>(s_i));
    }

    printf("  PASS\n");
}

void test_parse_garbage() {
    printf("Test 3: Garbage data does not crash\n");

    auto s_transport = std::make_unique<MockTransport>();
    auto* s_mock = static_cast<MockTransport*>(s_transport.get());
    ArmImpl s_impl(std::move(s_transport));

    // Inject random garbage
    std::vector<uint8_t> s_garbage(256, 0xFF);
    s_mock->inject(s_garbage);

    // Follow with a valid frame
    fci::arm::ArmStatus s_status{};
    s_status.seq = 999;
    s_status.status.q[0] = 7.5f;
    auto s_frame = serializeArmStatus(s_status);
    s_mock->inject(s_frame);

    auto s_state = s_impl.readOnce();
    assert(s_state.m_seq == 999);
    assert(s_state.m_q[0] == 7.5f);

    printf("  PASS\n");
}

int main() {
    test_arm_status_roundtrip();
    test_multiple_frames();
    test_parse_garbage();
    printf("\nAll tests passed.\n");
    return 0;
}
