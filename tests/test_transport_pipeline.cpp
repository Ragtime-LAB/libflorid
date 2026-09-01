#include "florid/Arm.hpp"
#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/Seqlock.hpp"
#include "florid/detail/UdpTransport.hpp"

#include "fci_protocol/arm/packets.hpp"
#include "fci_protocol/session/arm_control_session.hpp"
#include "fci_protocol/transport/byte_stream_transport.hpp"

#include <asio.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <type_traits>
#include <chrono>

using namespace florid;

static_assert(
    std::is_same_v<decltype(Version::m_major), std::uint32_t>);
static_assert(std::is_same_v<decltype(&Arm::deviceInfo),
                             const DeviceInfo& (Arm::*)() const>);
static_assert(std::is_same_v<decltype(&Arm::deviceSettings),
                             const DeviceSettings& (Arm::*)() const>);
static_assert(std::is_same_v<decltype(&Arm::readDiagnostics),
                             ArmDiagnostics (Arm::*)()>);

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

std::vector<uint8_t> s_makeDeviceInfoFrame(std::uint8_t s_req_id,
                                            bool s_invalid = false) {
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
    constexpr char s_board_name[] = "willow";
    constexpr char s_custom_name[] = "机械臂";
    std::memcpy(s_frame.data() + 12, s_board_name, sizeof(s_board_name));
    std::memcpy(s_frame.data() + 44, s_custom_name, sizeof(s_custom_name));
    s_frame[76] = 1;                            // MobileArm
    if (s_invalid) {
        // Legacy fixed strings must be NUL-terminated and have zero padding.
        // Preserve the valid prefix, but reject a non-zero byte after the NUL.
        s_frame[43] = 'x';
        s_frame[76] = 0xfe;
    }
    return s_frame;
}

std::vector<uint8_t> s_makeDeviceSettingsFrame(std::uint8_t s_req_id) {
    // Frame: 5-byte header + 113-byte payload = 118 bytes
    // Payload: req_id(1) + firmware_dt_us(4) + gravity_scale(24) + torque_fold(84)
    std::vector<uint8_t> s_frame(118, 0);
    s_frame[0] = 0xA5;
    s_frame[1] = 0x71;  s_frame[2] = 0x00;     // length = 113
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
    explicit MockTransport(bool s_invalid_device_info = false)
        : m_invalid_device_info(s_invalid_device_info) {}

    bool send(const std::uint8_t* s_data, std::size_t s_size) override {
        m_sent.insert(m_sent.end(), s_data, s_data + s_size);

        // Auto-respond to GetDeviceInfoRequest so ArmImpl construction succeeds
        if (s_size >= 7 && s_data[0] == 0xA5) {
            std::uint16_t s_cmd = static_cast<std::uint16_t>(s_data[3])
                               | (static_cast<std::uint16_t>(s_data[4]) << 8);
            if (s_cmd == 0x6215) {
                std::uint8_t s_req_id = s_data[5];
                inject(s_makeAckFrame(s_req_id));
                inject(s_makeDeviceInfoFrame(s_req_id, m_invalid_device_info));
            }
            if (s_cmd == 0x6228) {
                std::uint8_t s_req_id = s_data[5];
                inject(s_makeAckFrame(s_req_id));
                inject(s_makeDeviceSettingsFrame(s_req_id));
            }
            if (s_cmd == 0x622A) {
                inject(s_makeAckFrame(s_data[5]));
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
    bool m_invalid_device_info{};
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

std::vector<std::uint8_t> serializeDiagnostics(
    fci::arm::ArmDiagnostics& s_diagnostics) {
    using Session = RPL::USBTransport<
        RPL::AckManager<DummyTick>,
        std::function<void(const std::uint8_t*, std::size_t)>,
        USBAck,
        fci::arm::ArmDiagnostics>;

    std::vector<std::uint8_t> s_bytes;
    Session s_session;
    s_session.on_send([&s_bytes](const std::uint8_t* s_data,
                                 std::size_t s_size) {
        s_bytes.assign(s_data, s_data + s_size);
    });
    const auto s_result = s_session.notify(s_diagnostics);
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
    assert(s_impl.getDeviceInfo().m_protocol_version.m_major == 1);
    assert(s_impl.getDeviceInfo().m_firmware_version.m_major == 2);
    assert(s_impl.getDeviceInfo().m_firmware_version.m_minor == 3);
    assert(s_impl.getDeviceInfo().m_firmware_version.m_patch == 1);
    assert(s_impl.getDeviceInfo().m_board_name == "willow");
    assert(s_impl.getDeviceInfo().m_custom_name == "机械臂");
    assert(s_impl.getDeviceInfo().m_firmware_type ==
           FirmwareType::kMobileArm);

    // Verify device settings were fetched
    assert(s_impl.getDeviceSettings().m_firmware_period_us == 2000);

    auto s_settings = s_impl.getDeviceSettings();
    s_settings.m_firmware_period_us = 1000;
    s_settings.m_gravity_scale[0] = 1.25f;
    s_settings.m_torque_fold[0] = TorqueFoldParameters{
        .m_continuous_torque = 1.0f,
        .m_peak_torque = 2.0f,
        .m_thermal_capacity = 3.0f,
        .m_torque_ramp_rate = 4.0f,
    };
    s_settings.m_joint_limits[0] = JointLimits{.m_min = -1.0f,
                                               .m_max = 1.0f};
    assert(s_impl.setDeviceSettings(s_settings));
    assert(s_impl.getDeviceSettings().m_firmware_period_us == 1000);
    assert(s_impl.firmwarePeriodUs() == 1000);

    const auto s_sent_before_invalid = s_mock->m_sent.size();
    auto s_invalid_settings = s_settings;
    s_invalid_settings.m_gravity_scale[0] =
        std::numeric_limits<float>::quiet_NaN();
    assert(!s_impl.setDeviceSettings(s_invalid_settings));
    s_invalid_settings = s_settings;
    s_invalid_settings.m_joint_limits[0] = JointLimits{.m_min = 2.0f,
                                                       .m_max = 1.0f};
    assert(!s_impl.setDeviceSettings(s_invalid_settings));
    assert(s_mock->m_sent.size() == s_sent_before_invalid);

    assert(!s_impl.readMotorRegister(0, MotorRegister::SpeedLoopKp));
    assert(!s_impl.writeMotorRegister(
        1, static_cast<MotorRegister>(0xff), 1.0f));
    assert(!s_impl.writeMotorRegister(
        1, MotorRegister::SpeedLoopKp,
        std::numeric_limits<float>::infinity()));
    assert(s_mock->m_sent.size() == s_sent_before_invalid);

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

    fci::arm::ArmDiagnostics s_diagnostics{};
    s_diagnostics.uptime_s = 10;
    s_diagnostics.tick_count = 5;
    s_diagnostics.mode_entry_ms = 20;
    s_diagnostics.bus_healthy = 1;
    s_diagnostics.bus_state = 3;
    s_diagnostics.tx_err_count = 2;
    s_diagnostics.rx_err_count = 4;
    s_diagnostics.joints[0].healthy = 1;
    s_diagnostics.joints[0].temp_c = 42.5f;
    s_diagnostics.gripper.healthy = 1;
    s_diagnostics.gripper.temp_c = 38.0f;
    auto s_diagnostics_frame = serializeDiagnostics(s_diagnostics);
    s_mock->inject(s_diagnostics_frame);
    auto s_domain_diagnostics = s_impl.readDiagnostics();
    assert(s_domain_diagnostics.m_tick_count == 5);
    assert(s_domain_diagnostics.m_bus_healthy);
    assert(s_domain_diagnostics.m_bus_state == BusState::kBusOff);
    assert(s_domain_diagnostics.m_joints[0].m_healthy);
    assert(s_domain_diagnostics.m_joints[0].m_temperature_c == 42.5f);

    s_diagnostics.tick_count = 6;
    s_diagnostics.bus_healthy = 2;
    s_diagnostics.bus_state = 0xfe;
    s_diagnostics.joints[0].healthy = 2;
    s_diagnostics.joints[0].temp_c =
        std::numeric_limits<float>::quiet_NaN();
    s_diagnostics_frame = serializeDiagnostics(s_diagnostics);
    s_mock->inject(s_diagnostics_frame);
    s_domain_diagnostics = s_impl.readDiagnostics();
    assert(!s_domain_diagnostics.m_bus_healthy);
    assert(s_domain_diagnostics.m_bus_state == BusState::kUnknown);
    assert(!s_domain_diagnostics.m_joints[0].m_healthy);
    assert(s_domain_diagnostics.m_joints[0].m_temperature_c == 0.0f);

    printf("  PASS\n");
}

void test_device_info_validation() {
    printf("Test 2: DeviceInfo bridge validates legacy strings and enums\n");

    auto s_transport = std::make_unique<MockTransport>(true);
    ArmImpl s_impl(std::move(s_transport));
    assert(s_impl.getDeviceInfo().m_board_name.empty());
    assert(s_impl.getDeviceInfo().m_custom_name == "机械臂");
    assert(s_impl.getDeviceInfo().m_firmware_type == FirmwareType::kUnknown);

    printf("  PASS\n");
}

void test_multiple_frames() {
    printf("Test 3: Multiple sequential ArmStatus frames\n");

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
    printf("Test 4: Garbage data does not crash\n");

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
    printf("Test 5: Control loop sends commands\n");

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
    printf("Test 6: UDP loopback with FakeUdpDevice\n");

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
        assert(s_impl.getDeviceInfo().m_firmware_version.m_major == 2);

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
    test_device_info_validation();
    test_multiple_frames();
    test_parse_garbage();
    test_control_loop();
    test_udp_loopback();
    printf("\nAll tests passed.\n");
    return 0;
}
