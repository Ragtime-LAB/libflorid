#include "florid/detail/FciWirelinkEndpoint.hpp"

#include "fci_arm_bindings.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {

using namespace std::chrono_literals;
using florid::DeviceInfo;
using florid::JointMIT;
using florid::detail::FciEndpointStatus;
using florid::detail::FciOperationResult;
using florid::detail::FciOperationState;
using florid::detail::FciWirelinkEndpoint;
using florid::detail::FciWirelinkEndpointConfig;
using florid::detail::WirelinkExecutor;
using florid::detail::WirelinkExecutorHooks;

void require(bool s_condition, const char* s_message) {
    if (!s_condition) throw std::runtime_error(s_message);
}

template <typename Predicate>
void waitFor(std::condition_variable& s_cv, std::mutex& s_mutex,
             Predicate s_predicate, const char* s_message) {
    std::unique_lock<std::mutex> s_lock(s_mutex);
    require(s_cv.wait_for(s_lock, 3s, s_predicate), s_message);
}

struct PeerStorage {
    std::array<std::uint8_t, 256> m_tx_payload{};
    std::array<std::uint8_t, 320> m_tx_unit{};
    std::array<std::uint8_t, 64> m_control_unit{};
    std::array<std::uint8_t, 4096> m_rx_fifo{};
    std::array<std::uint8_t, 320> m_rx_fallback{};

    wl_storage_t descriptor() noexcept {
        return wl_storage_t{
            .tx_payload = m_tx_payload.data(),
            .tx_payload_size = m_tx_payload.size(),
            .tx_unit = m_tx_unit.data(),
            .tx_unit_size = m_tx_unit.size(),
            .control_unit = m_control_unit.data(),
            .control_unit_size = m_control_unit.size(),
            .rx_fifo = m_rx_fifo.data(),
            .rx_fifo_size = m_rx_fifo.size(),
            .rx_fallback = m_rx_fallback.data(),
            .rx_fallback_size = m_rx_fallback.size(),
        };
    }
};

struct FragmentSink {
    FciWirelinkEndpoint* m_endpoint{};
    WirelinkExecutor* m_executor{};
    std::size_t m_chunk_size{3};
    std::atomic<bool> m_busy{};
    std::atomic<std::uint64_t> m_fragments{};
    std::atomic<std::uint64_t> m_failures{};

    static wl_sink_result_t sink(void* s_user_data, wl_io_token_t,
                                 const std::uint8_t* s_data,
                                 std::size_t s_size) noexcept {
        auto& s_self = *static_cast<FragmentSink*>(s_user_data);
        if (s_self.m_busy.load(std::memory_order_acquire)) {
            return WL_SINK_BUSY;
        }
        std::size_t s_offset{};
        while (s_offset < s_size) {
            const std::size_t s_chunk =
                std::min(s_self.m_chunk_size, s_size - s_offset);
            std::size_t s_accepted{};
            bool s_ok{};
            if (s_self.m_endpoint != nullptr) {
                s_ok = s_self.m_endpoint->feedBytes(
                           s_data + s_offset, s_chunk, s_accepted) ==
                           FciEndpointStatus::kOk;
            } else {
                s_ok = s_self.m_executor->feedBytes(
                           s_data + s_offset, s_chunk, s_accepted) == WL_OK;
            }
            if (!s_ok || s_accepted != s_chunk) {
                s_self.m_failures.fetch_add(1, std::memory_order_relaxed);
                return WL_SINK_FAILED;
            }
            s_self.m_fragments.fetch_add(1, std::memory_order_relaxed);
            s_offset += s_chunk;
        }
        return WL_SINK_SENT;
    }
};

class DevicePeer {
public:
    int initialize(std::uint64_t s_session_id) {
        const wl_config_t s_config{
            .max_payload_len = 256,
            .envelope = WL_ENVELOPE_COBS_STREAM,
            .integrity = WL_INTEGRITY_NONE,
            .session_id = s_session_id,
            .max_retries = 2,
            .ack_timeout_ms = 20,
            .max_transmission_unit = 320,
        };
        auto s_storage = m_storage.descriptor();
        int s_result = m_executor.initialize(s_config, s_storage);
        if (s_result != WL_OK) return s_result;
        WirelinkExecutorHooks s_hooks{};
        s_hooks.m_user_data = this;
        s_hooks.m_on_event = s_onEvent;
        return m_executor.setHooks(s_hooks);
    }

    int setSink(wl_sink_fn s_sink, void* s_user_data) {
        return m_executor.setSink(s_sink, s_user_data);
    }
    int start() { return m_executor.start(); }
    void stop() { m_executor.stop(); }
    void notify() { m_executor.notify(); }

    int sendArmStatus(std::uint32_t s_sequence, float s_position) {
        arm_status_t s_status{};
        arm_status_clear(&s_status);
        s_status.has_mode = true;
        s_status.mode = ARM_MODE_PC;
        s_status.has_sequence = true;
        s_status.sequence = s_sequence;
        s_status.has_timestamp_us = true;
        s_status.timestamp_us = UINT64_C(1000000) + s_sequence;
        s_status.has_joint_position = true;
        s_status.has_joint_velocity = true;
        s_status.has_joint_torque = true;
        s_status.has_base_gravity = true;
        s_status.has_gripper_position = true;
        s_status.has_gripper_velocity = true;
        s_status.has_gripper_torque = true;
        s_status.has_end_effector_transform = true;
        s_status.has_external_wrench = true;
        s_status.has_error_flags = true;
        s_status.has_last_sdk_timestamp_us = true;
        s_status.last_sdk_timestamp_us = UINT64_C(900000) + s_sequence;
        for (std::size_t s_index = 0; s_index < 6; ++s_index) {
            s_status.joint_position[s_index] =
                s_position + static_cast<float>(s_index);
            s_status.joint_velocity[s_index] = 0.1F;
            s_status.joint_torque[s_index] = 0.2F;
            s_status.external_wrench[s_index] = 0.3F;
        }
        s_status.base_gravity[2] = -9.81F;
        s_status.end_effector_transform[0] = 1.0F;
        s_status.end_effector_transform[5] = 1.0F;
        s_status.end_effector_transform[10] = 1.0F;
        s_status.end_effector_transform[15] = 1.0F;

        std::array<std::uint8_t, 256> s_payload{};
        std::size_t s_size{};
        if (arm_status_encode(&s_status, s_payload.data(), s_payload.size(),
                              &s_size) != WL_CODEC_OK) {
            return WL_ERR_INVALID_ARG;
        }
        return m_executor.submitLatest(ARM_STATUS_MESSAGE_ID,
                                       s_payload.data(), s_size);
    }

    WirelinkExecutor& executor() noexcept { return m_executor; }

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::uint64_t m_lease_token{UINT64_C(0x1122334455667788)};
    std::size_t m_acquire_requests{};
    std::size_t m_release_requests{};
    std::size_t m_info_requests{};
    std::size_t m_joint_commands{};
    joint_mit_command_t m_last_joint{};
    std::atomic<bool> m_drop_device_info{};
    std::atomic<std::uint64_t> m_send_failures{};

private:
    static void s_onEvent(void* s_user_data, wl_ctx_t& s_context,
                          const wl_event_t& s_event) noexcept {
        auto& s_self = *static_cast<DevicePeer*>(s_user_data);
        if (s_event.type != WL_EVT_UNRELIABLE_RX &&
            s_event.type != WL_EVT_RELIABLE_RX) {
            return;
        }

        switch (s_event.message_id) {
            case ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID:
                s_self.s_acquire(s_context, s_event);
                break;
            case RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID:
                s_self.s_release(s_context, s_event);
                break;
            case GET_DEVICE_INFO_REQUEST_MESSAGE_ID:
                s_self.s_deviceInfo(s_context, s_event);
                break;
            case JOINT_MIT_COMMAND_MESSAGE_ID:
                s_self.s_joint(s_event);
                break;
            default:
                break;
        }
        wl_event_release(&s_context, &s_event);
    }

    void s_recordSend(const fci_arm_send_result_t& s_result) noexcept {
        if (s_result.domain != FCI_ARM_SEND_OK ||
            s_result.core_result != WL_OK) {
            m_send_failures.fetch_add(1, std::memory_order_relaxed);
        }
    }

    static fci_arm_encode_scratch_t s_scratch(
        std::array<std::uint8_t, 256>& s_storage) noexcept {
        return {s_storage.data(), s_storage.size()};
    }

    void s_acquire(wl_ctx_t& s_context,
                   const wl_event_t& s_event) noexcept {
        acquire_control_lease_request_t s_request{};
        if (acquire_control_lease_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id ||
            !s_request.has_requested_timeout_ms) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_acquire_requests;
        }
        acquire_control_lease_response_t s_response{};
        acquire_control_lease_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = CONTROL_LEASE_OK;
        s_response.has_lease_token = true;
        s_response.lease_token = m_lease_token;
        s_response.has_granted_timeout_ms = true;
        s_response.granted_timeout_ms = s_request.requested_timeout_ms;
        std::array<std::uint8_t, 256> s_encode{};
        s_recordSend(fci_arm_acquire_control_lease_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
        m_cv.notify_all();
    }

    void s_release(wl_ctx_t& s_context,
                   const wl_event_t& s_event) noexcept {
        release_control_lease_request_t s_request{};
        if (release_control_lease_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id || !s_request.has_lease_token) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_release_requests;
        }
        release_control_lease_response_t s_response{};
        release_control_lease_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = s_request.lease_token == m_lease_token
                                ? CONTROL_LEASE_OK
                                : CONTROL_LEASE_INVALID_TOKEN;
        std::array<std::uint8_t, 256> s_encode{};
        s_recordSend(fci_arm_release_control_lease_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
        m_cv.notify_all();
    }

    void s_deviceInfo(wl_ctx_t& s_context,
                      const wl_event_t& s_event) noexcept {
        get_device_info_request_t s_request{};
        if (get_device_info_request_decode(s_event.payload,
                                           s_event.payload_len,
                                           &s_request) != WL_CODEC_OK ||
            !s_request.has_operation_id) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_info_requests;
        }
        m_cv.notify_all();
        if (m_drop_device_info.load(std::memory_order_acquire)) return;

        constexpr char s_board[] = "ESP32-S3";
        constexpr char s_custom[] = "arm-\xE4\xB8\x80";
        get_device_info_response_t s_response{};
        get_device_info_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = DEVICE_INFO_OK;
        s_response.has_info = true;
        auto& s_info = s_response.info;
        s_info.has_protocol_version = true;
        s_info.protocol_version.has_major = true;
        s_info.protocol_version.major = 1;
        s_info.protocol_version.has_minor = true;
        s_info.protocol_version.minor = 2;
        s_info.protocol_version.has_patch = true;
        s_info.protocol_version.patch = 3;
        s_info.has_firmware_version = true;
        s_info.firmware_version.has_major = true;
        s_info.firmware_version.major = 4;
        s_info.firmware_version.has_minor = true;
        s_info.firmware_version.minor = 5;
        s_info.firmware_version.has_patch = true;
        s_info.firmware_version.patch = 6;
        s_info.has_board_name = true;
        s_info.board_name = {s_board, sizeof(s_board) - 1};
        s_info.has_custom_name = true;
        s_info.custom_name = {s_custom, sizeof(s_custom) - 1};
        s_info.has_firmware_type = true;
        s_info.firmware_type = FIRMWARE_COBOT_ARM;
        std::array<std::uint8_t, 256> s_encode{};
        s_recordSend(fci_arm_get_device_info_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
    }

    void s_joint(const wl_event_t& s_event) noexcept {
        joint_mit_command_t s_command{};
        if (joint_mit_command_decode(s_event.payload, s_event.payload_len,
                                     &s_command) != WL_CODEC_OK) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            m_last_joint = s_command;
            ++m_joint_commands;
        }
        m_cv.notify_all();
    }

    PeerStorage m_storage;
    WirelinkExecutor m_executor;
};

struct StatusCapture {
    std::mutex m_mutex;
    std::condition_variable m_cv;
    florid::detail::FciArmStatusSnapshot m_status{};
    std::size_t m_calls{};

    static void callback(
        void* s_user_data,
        const florid::detail::FciArmStatusSnapshot& s_status) noexcept {
        auto& s_self = *static_cast<StatusCapture*>(s_user_data);
        {
            std::lock_guard<std::mutex> s_lock(s_self.m_mutex);
            s_self.m_status = s_status;
            ++s_self.m_calls;
        }
        s_self.m_cv.notify_all();
    }
};

void testTypedEndpointLifecycle() {
    FciWirelinkEndpoint s_host;
    DevicePeer s_device;
    require(s_host.initialize(FciWirelinkEndpointConfig{
                .m_session_id = UINT64_C(0x1000000000000001),
                .m_max_retries = 2,
                .m_ack_timeout_ms = 20,
            }) == FciEndpointStatus::kOk,
            "host endpoint initialization failed");
    require(s_device.initialize(UINT64_C(0x2000000000000002)) == WL_OK,
            "device peer initialization failed");

    FragmentSink s_host_to_device{
        .m_executor = &s_device.executor(),
        .m_chunk_size = 2,
    };
    FragmentSink s_device_to_host{
        .m_endpoint = &s_host,
        .m_chunk_size = 3,
    };
    require(s_host.setSink(FragmentSink::sink, &s_host_to_device) ==
                FciEndpointStatus::kOk,
            "host sink setup failed");
    require(s_device.setSink(FragmentSink::sink, &s_device_to_host) == WL_OK,
            "device sink setup failed");
    StatusCapture s_status_capture;
    require(s_host.setCallbacks(StatusCapture::callback, nullptr,
                                &s_status_capture) == FciEndpointStatus::kOk,
            "host callback setup failed");
    require(s_device.start() == WL_OK, "device peer start failed");
    require(s_host.start() == FciEndpointStatus::kOk,
            "host endpoint start failed");

    const auto s_info_request = s_host.getDeviceInfo(250);
    require(s_info_request.m_status == FciEndpointStatus::kOk,
            "GetDeviceInfo was not queued");
    FciOperationResult s_operation{};
    const auto s_info_wait = s_host.waitOperation(
        s_info_request.m_request_id, 1s, s_operation);
    if (s_info_wait != FciEndpointStatus::kOk) {
        std::fprintf(stderr,
                     "GetDeviceInfo wait=%u state=%u status=%u domain=%d "
                     "link=%d peer_requests=%zu send_failures=%llu\n",
                     static_cast<unsigned>(s_info_wait),
                     static_cast<unsigned>(s_operation.m_state),
                     static_cast<unsigned>(s_operation.m_status),
                     s_operation.m_domain_status, s_operation.m_link_status,
                     s_device.m_info_requests,
                     static_cast<unsigned long long>(
                         s_device.m_send_failures.load()));
    }
    require(s_info_wait == FciEndpointStatus::kOk,
            "GetDeviceInfo did not complete");
    DeviceInfo s_info{};
    require(s_host.takeDeviceInfo(s_info_request.m_request_id, s_operation,
                                  s_info) == FciEndpointStatus::kOk,
            "GetDeviceInfo result was not consumable");
    require(s_info.m_protocol_version.m_major == 1 &&
                s_info.m_firmware_version.m_patch == 6 &&
                s_info.m_board_name == "ESP32-S3" &&
                s_info.m_custom_name == "arm-\xE4\xB8\x80" &&
                s_info.m_firmware_type == florid::FirmwareType::kCobotArm,
            "GetDeviceInfo wire-to-domain conversion failed");

    const auto s_acquire = s_host.acquireControlLease(5000, 250);
    require(s_acquire.m_status == FciEndpointStatus::kOk,
            "control lease was not queued");
    require(s_host.waitOperation(s_acquire.m_request_id, 1s, s_operation) ==
                FciEndpointStatus::kOk,
            "control lease did not complete");
    require(s_host.takeOperation(s_acquire.m_request_id, s_operation) ==
                FciEndpointStatus::kOk,
            "control lease operation was not released");
    const auto s_lease = s_host.controlLease();
    require(s_lease.m_token == s_device.m_lease_token,
            "control lease token mismatch");

    JointMIT s_joint{};
    s_joint.m_q[0] = 1.0F;
    s_joint.m_kp[0] = 10.0F;
    s_joint.m_kd[0] = 0.5F;
    require(s_host.sendJointMit(s_joint, 2000, 100) ==
                FciEndpointStatus::kOk,
            "JointMIT send failed");
    waitFor(s_device.m_cv, s_device.m_mutex,
            [&] { return s_device.m_joint_commands == 1; },
            "JointMIT did not reach the peer");
    require(s_device.m_last_joint.has_lease_token &&
                s_device.m_last_joint.lease_token == s_device.m_lease_token,
            "JointMIT omitted the lease token");

    s_host_to_device.m_busy.store(true, std::memory_order_release);
    for (int s_index = 2; s_index <= 20; ++s_index) {
        s_joint.m_q[0] = static_cast<float>(s_index);
        require(s_host.sendJointMit(s_joint, 2000,
                                    static_cast<std::uint64_t>(s_index)) ==
                    FciEndpointStatus::kOk,
                "coalesced JointMIT submit failed");
    }
    std::this_thread::sleep_for(20ms);
    s_host_to_device.m_busy.store(false, std::memory_order_release);
    s_host.notify();
    waitFor(s_device.m_cv, s_device.m_mutex,
            [&] {
                return s_device.m_joint_commands >= 2 &&
                       s_device.m_last_joint.position[0] == 20.0F;
            },
            "coalesced JointMIT did not reach the peer");
    require(s_device.m_joint_commands <= 3,
            "LATEST leaked more than one already-staged value");
    require(s_host.executorStats().m_latest_coalesced >= 18,
            "LATEST replacement was not recorded");

    require(s_device.sendArmStatus(41, 4.25F) == WL_OK,
            "ArmStatus submit failed");
    waitFor(s_status_capture.m_cv, s_status_capture.m_mutex,
            [&] { return s_status_capture.m_calls == 1; },
            "ArmStatus callback did not run");
    florid::detail::FciArmStatusSnapshot s_snapshot{};
    require(s_host.latestArmStatus(s_snapshot) == FciEndpointStatus::kOk &&
                s_snapshot.m_state.m_seq == 41 &&
                s_snapshot.m_state.m_q[0] == 4.25F,
            "ArmStatus snapshot was not stable after borrowed release");

    const auto s_release = s_host.releaseControlLease(250);
    require(s_release.m_status == FciEndpointStatus::kOk,
            "lease release was not queued");
    require(s_host.waitOperation(s_release.m_request_id, 1s, s_operation) ==
                FciEndpointStatus::kOk &&
                s_host.takeOperation(s_release.m_request_id, s_operation) ==
                    FciEndpointStatus::kOk,
            "lease release did not complete");
    require(s_host.sendJointMit(s_joint, 2000, 300) ==
                FciEndpointStatus::kNoLease,
            "command was accepted without a lease");

    s_device.m_drop_device_info.store(true, std::memory_order_release);
    const auto s_timed = s_host.getDeviceInfo(45);
    const auto s_poll_before = s_host.stats().m_runtime_poll_calls;
    require(s_host.waitOperation(s_timed.m_request_id, 1s, s_operation) ==
                FciEndpointStatus::kTimeout &&
                s_operation.m_state == FciOperationState::kTimedOut,
            "dropped RPC response did not become an explicit timeout");
    require(s_host.takeOperation(s_timed.m_request_id, s_operation) ==
                FciEndpointStatus::kTimeout,
            "timed out RPC was not releasable");
    const auto s_poll_after = s_host.stats().m_runtime_poll_calls;
    require(s_poll_after - s_poll_before < 20,
            "RPC deadline handling busy-spun");
    std::this_thread::sleep_for(30ms);
    require(s_host.stats().m_runtime_poll_calls == s_poll_after,
            "idle endpoint retained a hidden periodic wakeup");

    s_host_to_device.m_busy.store(true, std::memory_order_release);
    const auto s_pending = s_host.getDeviceInfo(500);
    require(s_pending.m_status == FciEndpointStatus::kOk,
            "pending shutdown RPC was not queued");
    std::this_thread::sleep_for(10ms);
    const auto s_stop_started = std::chrono::steady_clock::now();
    s_host.stop();
    require(std::chrono::steady_clock::now() - s_stop_started < 500ms,
            "endpoint shutdown waited for the RPC deadline");
    require(s_host.waitOperation(s_pending.m_request_id, 0ms, s_operation) ==
                FciEndpointStatus::kCancelled &&
                s_host.takeOperation(s_pending.m_request_id, s_operation) ==
                    FciEndpointStatus::kCancelled,
            "shutdown did not cancel and release pending RPC state");
    s_device.stop();

    const auto s_stats = s_host.stats();
    require(s_stats.m_latest_acquires == s_stats.m_latest_releases &&
                s_stats.m_latest_acquires != 0,
            "retained telemetry was not released exactly once");
    require(s_stats.m_rpc_started == s_stats.m_rpc_released &&
                s_stats.m_rpc_cancelled == 1,
            "RPC runtime slots were not released exactly once");
    if (s_stats.m_dispatch_errors != 0 ||
        s_stats.m_runtime_storage_bytes > 4096 ||
        s_stats.m_runtime_storage_bytes < 4000) {
        std::fprintf(stderr,
                     "dispatch_errors=%llu runtime_storage=%zu "
                     "rpc_started=%llu rpc_released=%llu\n",
                     static_cast<unsigned long long>(
                         s_stats.m_dispatch_errors),
                     s_stats.m_runtime_storage_bytes,
                     static_cast<unsigned long long>(s_stats.m_rpc_started),
                     static_cast<unsigned long long>(s_stats.m_rpc_released));
    }
    require(s_stats.m_dispatch_errors == 0 &&
                s_stats.m_runtime_storage_bytes <= 4096 &&
                s_stats.m_runtime_storage_bytes >= 4000,
            "generated runtime dispatch or bounded storage sizing failed");
    require(s_host_to_device.m_failures.load() == 0 &&
                s_device_to_host.m_failures.load() == 0 &&
                s_host_to_device.m_fragments.load() > 10 &&
                s_device_to_host.m_fragments.load() > 10 &&
                s_device.m_send_failures.load() == 0,
            "fragmented COBS link failed");
}

} // namespace

int main() {
    try {
        testTypedEndpointLifecycle();
        std::puts("PASS: typed FCI endpoint owns runtime, RPC, LATEST, and shutdown");
        return 0;
    } catch (const std::exception& s_error) {
        std::fprintf(stderr, "FAIL: %s\n", s_error.what());
        return 1;
    }
}
