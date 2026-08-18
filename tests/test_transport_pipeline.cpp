#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/Seqlock.hpp"
#include "florid/detail/UdpTransport.hpp"

#include "fci_protocol/arm/packets.hpp"
#include "fci_protocol/session/arm_control_session.hpp"
#include "fci_protocol/transport/byte_stream_transport.hpp"

#include <asio.hpp>

#include <array>
#include <cassert>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>

using namespace florid;

#ifdef _WIN32
void s_disableUdpConnReset(asio::ip::udp::socket& s_socket) {
    constexpr DWORD s_kSioUdpConnReset = 0x9800000C;
    DWORD s_zero = 0;
    DWORD s_returned = 0;
    (void)::WSAIoctl(s_socket.native_handle(), s_kSioUdpConnReset, &s_zero,
                     sizeof(s_zero), nullptr, 0, &s_returned, nullptr, nullptr);
}
#endif

// ────────────────────────────────────────────────────────────
//  Frame builders (shared by MockTransport and FakeUdpDevice)
// ────────────────────────────────────────────────────────────

std::vector<uint8_t> s_makeAckFrame(std::uint8_t s_req_id) {
    return {
        0xA5, 0x02, 0x00,             // start + length=2
        0x00, 0x00,                   // cmd = USBAck (0x0000)
        s_req_id, 0x00                // status = Ok
    };
}

std::vector<uint8_t> s_makeDeviceInfoFrame(std::uint8_t s_req_id) {
    // Frame: 5-byte header + 72-byte payload = 77 bytes
    // Payload: req_id(1) + protocol_version(3) + fw_version(3)
    //          + board_name(32) + custom_name(32) + fw_type(1)
    std::vector<uint8_t> s_frame(77, 0);
    s_frame[0] = 0xA5;
    s_frame[1] = 0x48;  s_frame[2] = 0x00;     // length = 72
    s_frame[3] = 0x16;  s_frame[4] = 0x62;     // cmd = 0x6216 (GetDeviceInfoResponse)
    s_frame[5] = s_req_id;
    s_frame[6] = 1;                             // protocol_version.major
    s_frame[9] = 2;                             // fw_version.major
    s_frame[10] = 3;                            // fw_version.minor
    s_frame[11] = 1;                            // fw_version.patch
    return s_frame;
}

std::vector<uint8_t> s_makeDeviceSettingsFrame(std::uint8_t s_req_id) {
    // Frame: 5-byte header + 189-byte payload = 194 bytes
    // Payload: req_id(1) + firmware_dt_us(4) + gravity_scale(24)
    //          + torque_fold(7x16=112) + joint_limits(48)
    std::vector<uint8_t> s_frame(5 + 189, 0);
    s_frame[0] = 0xA5;
    s_frame[1] = 0xBD;  s_frame[2] = 0x00;     // length = 189
    s_frame[3] = 0x29;  s_frame[4] = 0x62;     // cmd = 0x6229 (GetDeviceSettingsResponse)
    s_frame[5] = s_req_id;
    s_frame[6] = 0xD0; s_frame[7] = 0x07;      // firmware_dt_us = 2000 (LE)
    s_frame[8] = 0x00; s_frame[9] = 0x00;      // firmware_dt_us continued
    return s_frame;
}

// ────────────────────────────────────────────────────────────
//  MockTransport: captures sent bytes, can inject received bytes
// ────────────────────────────────────────────────────────────

class MockTransport : public Transport {
public:
    bool send(const std::uint8_t* s_data, std::size_t s_size) override {
        m_sent.insert(m_sent.end(), s_data, s_data + s_size);

        // Auto-respond to GetDeviceInfoRequest so ArmImpl construction succeeds
        if (s_size >= 7 && s_data[0] == 0xA5) {
            std::uint16_t s_cmd = static_cast<std::uint16_t>(s_data[3])
                               | (static_cast<std::uint16_t>(s_data[4]) << 8);
            if (s_cmd == 0x6215) {
                std::uint8_t s_req_id = s_data[5];
                inject(s_makeAckFrame(s_req_id));
                inject(s_makeDeviceInfoFrame(s_req_id));
            }
            if (s_cmd == 0x6228) {
                std::uint8_t s_req_id = s_data[5];
                inject(s_makeAckFrame(s_req_id));
                inject(s_makeDeviceSettingsFrame(s_req_id));
            }
        }
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
//  FakeUdpDevice: real UDP peer that answers the SDK handshake and
//  can push ArmStatus frames to the SDK's bound endpoint.
// ────────────────────────────────────────────────────────────

class FakeUdpDevice {
public:
    FakeUdpDevice(const asio::ip::udp::endpoint& s_bind, asio::ip::udp::endpoint s_sdk)
        : m_ctx(1), m_socket(m_ctx), m_sdk(std::move(s_sdk)) {
        m_socket.open(s_bind.protocol());
#ifdef _WIN32
        s_disableUdpConnReset(m_socket);
#endif
        m_socket.bind(s_bind);
        m_work_guard = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
            m_ctx.get_executor());
        m_thread = std::jthread([this] { m_ctx.run(); });
        start_receive();
    }

    ~FakeUdpDevice() {
        m_closed.store(true, std::memory_order_release);
        asio::error_code s_ec;
        m_socket.close(s_ec);
        m_ctx.stop();
        if (m_thread.joinable()) m_thread.join();
    }

    asio::ip::udp::endpoint localEndpoint() const { return m_socket.local_endpoint(); }

    void send(const std::vector<uint8_t>& s_bytes) {
        std::lock_guard<std::mutex> s_lock(m_send_mutex);
        asio::error_code s_ec;
        m_socket.send_to(asio::buffer(s_bytes), m_sdk, 0, s_ec);
    }

private:
    void start_receive() {
        if (m_closed.load(std::memory_order_acquire)) return;
        try {
            m_socket.async_receive_from(
                asio::buffer(m_rx_buffer), m_remote,
                [this](const asio::error_code& s_ec, std::size_t s_bytes) {
                    if (s_ec) {
                        if (s_ec != asio::error::operation_aborted) {
                            start_receive();
                        }
                        return;
                    }
                    handle(s_bytes);
                    start_receive();
                });
        } catch (...) {
        }
    }

    void handle(std::size_t s_bytes) {
        if (s_bytes >= 7 && m_rx_buffer[0] == 0xA5) {
            std::uint16_t s_cmd = static_cast<std::uint16_t>(m_rx_buffer[3])
                                | (static_cast<std::uint16_t>(m_rx_buffer[4]) << 8);
            std::uint8_t s_req_id = m_rx_buffer[5];
            if (s_cmd == 0x6215) {
                send(s_makeAckFrame(s_req_id));
                send(s_makeDeviceInfoFrame(s_req_id));
            } else if (s_cmd == 0x6228) {
                send(s_makeAckFrame(s_req_id));
                send(s_makeDeviceSettingsFrame(s_req_id));
            }
        }
    }

    asio::io_context m_ctx;
    asio::ip::udp::socket m_socket;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_work_guard;
    std::jthread m_thread;
    std::array<std::uint8_t, 2048> m_rx_buffer{};
    asio::ip::udp::endpoint m_remote;
    asio::ip::udp::endpoint m_sdk;
    std::mutex m_send_mutex;
    std::atomic<bool> m_closed{false};
};

std::uint16_t s_pickFreeUdpPort() {
    asio::io_context s_ctx;
    asio::ip::udp::socket s_sock(s_ctx);
    s_sock.open(asio::ip::udp::v4());
    s_sock.bind(asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
    std::uint16_t s_port = s_sock.local_endpoint().port();
    s_sock.close();
    return s_port;
}

// ────────────────────────────────────────────────────────────
//  Tests
// ────────────────────────────────────────────────────────────

void test_arm_status_roundtrip() {
    printf("Test 1: ArmStatus round-trip through MockTransport\n");

    auto s_transport = std::make_unique<MockTransport>();
    auto* s_mock = static_cast<MockTransport*>(s_transport.get());

    // Create ArmImpl — MockTransport auto-responds to GetDeviceInfoRequest
    ArmImpl s_impl(std::move(s_transport));

    // Verify device info was fetched from mock response
    assert(s_impl.firmwarePeriodUs() == 2000);
    assert(s_impl.getDeviceInfo().protocol_version.major == 1);
    assert(s_impl.getDeviceInfo().fw_version.major == 2);
    assert(s_impl.getDeviceInfo().fw_version.minor == 3);
    assert(s_impl.getDeviceInfo().fw_version.patch == 1);

    // Verify device settings were fetched
    assert(s_impl.getDeviceSettings().firmware_dt_us == 2000);

    // Build a fake ArmStatus
    fci::arm::ArmStatus s_status{};
    s_status.seq = 123;
    s_status.timestamp_us = 999000;
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

void test_control_loop() {
    printf("Test 4: Control loop sends commands\n");

    auto s_transport = std::make_unique<MockTransport>();
    auto* s_mock = static_cast<MockTransport*>(s_transport.get());
    ArmImpl s_impl(std::move(s_transport));

    // Prepare 5 ArmStatus frames
    for (uint32_t s_i = 0; s_i < 5; ++s_i) {
        fci::arm::ArmStatus s_status{};
        s_status.seq = s_i;
        s_status.status.q[0] = static_cast<float>(s_i);
        auto s_frame = serializeArmStatus(s_status);

        // Inject, then call readOnce in the same thread (no separate control thread)
        // The control loop runs on the caller's thread via s_controlLoop
        s_mock->inject(s_frame);
        s_mock->m_sent.clear(); // reset sent buffer between iterations
    }

    // Inject one more frame for the control loop to consume
    fci::arm::ArmStatus s_status{};
    s_status.seq = 100;
    s_status.status.q[0] = 1.0f;
    auto s_frame = serializeArmStatus(s_status);
    s_mock->inject(s_frame);

    // Run control loop with a simple torque callback
    int s_call_count = 0;
    s_impl.s_controlLoop([&](const ArmState& s_state, ArmControl&) -> JointMIT {
        s_call_count++;
        JointMIT s_cmd;
        s_cmd.m_tau[0] = s_state.m_q[0] * 10.0f;
        if (s_call_count >= 1) s_cmd.m_motion_finished = true;
        return s_cmd;
    });

    assert(s_call_count == 1);
    assert(!s_mock->m_sent.empty());

    printf("  PASS (callbacks=%d, sent_bytes=%zu)\n", s_call_count, s_mock->m_sent.size());
}

void test_udp_loopback() {
    printf("Test 5: UDP loopback with FakeUdpDevice\n");

    const auto s_sdk_addr = asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"),
                                                    s_pickFreeUdpPort());
    FakeUdpDevice s_device(asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 0),
                           s_sdk_addr);

    // Announce continuously until the transport learns our peer endpoint.
    std::atomic<bool> s_stop_announcer{false};
    std::jthread s_announcer([&] {
        fci::arm::ArmStatus s_announce{};
        s_announce.seq = 1;
        auto s_frame = serializeArmStatus(s_announce);
        while (!s_stop_announcer.load()) {
            s_device.send(s_frame);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // The transport constructor blocks until the first datagram (peer learn),
    // so build it on a background thread while the announcer feeds datagrams.
    std::unique_ptr<UdpTransport> s_transport;
    std::jthread s_builder([&] {
        s_transport = std::make_unique<UdpTransport>("127.0.0.1", s_sdk_addr.port(),
                                                     std::chrono::seconds(3));
    });
    s_builder.join();
    assert(s_transport != nullptr);

    {
        // Run the ArmImpl handshake over real UDP.
        ArmImpl s_impl(std::move(s_transport));
        s_stop_announcer.store(true);

        // Stop device traffic and let in-flight datagrams drain before the
        // ArmImpl (and its transport io thread) is torn down.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        assert(s_impl.firmwarePeriodUs() == 2000);
        assert(s_impl.getDeviceInfo().fw_version.major == 2);

        // Push a fresh ArmStatus frame over real UDP and read it back.
        fci::arm::ArmStatus s_status{};
        s_status.seq = 77;
        s_status.errors = 0x05;
        s_status.status.q[0] = 42.0f;
        auto s_frame = serializeArmStatus(s_status);
        s_device.send(s_frame);

        ArmState s_state{};
        for (int i = 0; i < 200 && s_state.m_seq != 77; ++i) {
            auto s_tmp = s_impl.readOnce();
            if (s_tmp.m_seq != 0) s_state = s_tmp;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        assert(s_state.m_seq == 77);
        assert(s_state.m_errors == 0x05);
        assert(s_state.m_q[0] == 42.0f);

        printf("  PASS (udp round-trip seq=%u q0=%f)\n", s_state.m_seq, s_state.m_q[0]);
    }

    s_announcer.join();
}

int main() {
    test_arm_status_roundtrip();
    test_multiple_frames();
    test_parse_garbage();
    test_control_loop();
    test_udp_loopback();
    printf("\nAll tests passed.\n");
    return 0;
}
