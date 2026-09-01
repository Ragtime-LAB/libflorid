#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/Transport.hpp"
#include "florid/detail/WirelinkExecutor.hpp"

#include "fci_arm_bindings.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;
using florid::ArmImpl;
using florid::DeviceSettings;
using florid::JointMIT;
using florid::JointPosVel;
using florid::MotorRegister;
using florid::Transport;
using florid::detail::WirelinkExecutor;
using florid::detail::WirelinkExecutorHooks;

void require(bool s_condition, const char* s_message) {
    if (!s_condition) throw std::runtime_error(s_message);
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

class DevicePeer;

class LoopbackTransport final : public Transport {
public:
    explicit LoopbackTransport(DevicePeer& s_peer) : m_peer(s_peer) {}

    bool send(const std::uint8_t* s_data, std::size_t s_size) override;

    void setReceiveCallback(ReceiveFunctor s_callback,
                            void* s_context) override {
        m_context.store(s_context, std::memory_order_relaxed);
        m_callback.store(s_callback, std::memory_order_release);
    }

    void poll() override {}

    bool deliver(const std::uint8_t* s_data, std::size_t s_size) noexcept {
        std::size_t s_offset{};
        while (s_offset < s_size) {
            const auto s_chunk = std::min<std::size_t>(3, s_size - s_offset);
            const auto s_callback =
                m_callback.load(std::memory_order_acquire);
            if (s_callback == nullptr) return false;
            s_callback(m_context.load(std::memory_order_relaxed),
                       s_data + s_offset, s_chunk);
            s_offset += s_chunk;
        }
        return true;
    }

private:
    DevicePeer& m_peer;
    std::atomic<ReceiveFunctor> m_callback{};
    std::atomic<void*> m_context{};
};

class DevicePeer {
public:
    int initialize() {
        const wl_config_t s_config{
            .max_payload_len = 256,
            .envelope = WL_ENVELOPE_COBS_STREAM,
            .integrity = WL_INTEGRITY_NONE,
            .session_id = UINT64_C(0x4455667788990011),
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

    int attach(LoopbackTransport& s_transport) {
        m_transport = &s_transport;
        return m_executor.setSink(s_sink, this);
    }

    int start() { return m_executor.start(); }
    void stop() { m_executor.stop(); }

    bool feed(const std::uint8_t* s_data, std::size_t s_size) noexcept {
        std::size_t s_offset{};
        while (s_offset < s_size) {
            const auto s_chunk = std::min<std::size_t>(2, s_size - s_offset);
            std::size_t s_accepted{};
            if (m_executor.feedBytes(s_data + s_offset, s_chunk,
                                     s_accepted) != WL_OK ||
                s_accepted != s_chunk) {
                return false;
            }
            s_offset += s_chunk;
        }
        return true;
    }

    int sendArmStatus(std::uint32_t s_sequence, float s_position) {
        arm_status_t s_status{};
        arm_status_clear(&s_status);
        s_status.has_mode = true;
        s_status.mode = ARM_MODE_PC;
        s_status.has_sequence = true;
        s_status.sequence = s_sequence;
        s_status.has_timestamp_us = true;
        s_status.timestamp_us = 10'000 + s_sequence;
        s_status.has_joint_position = true;
        s_status.has_joint_velocity = true;
        s_status.has_joint_torque = true;
        s_status.has_base_gravity = true;
        s_status.has_gripper_position = true;
        s_status.gripper_position = 0.25F;
        s_status.has_gripper_velocity = true;
        s_status.gripper_velocity = 0.5F;
        s_status.has_gripper_torque = true;
        s_status.gripper_torque = 0.75F;
        s_status.has_end_effector_transform = true;
        s_status.has_external_wrench = true;
        s_status.has_error_flags = true;
        s_status.has_last_sdk_timestamp_us = true;
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

    bool waitForCommand(std::uint16_t s_message_id) {
        std::unique_lock<std::mutex> s_lock(m_mutex);
        return m_changed.wait_for(s_lock, 2s, [&] {
            for (const auto s_seen : m_commands) {
                if (s_seen == s_message_id) return true;
            }
            return false;
        });
    }

    std::size_t acquireCount() const noexcept {
        return m_acquire_count.load(std::memory_order_relaxed);
    }
    std::size_t releaseCount() const noexcept {
        return m_release_count.load(std::memory_order_relaxed);
    }
    std::uint8_t lastRegisterJoint() const noexcept {
        return m_last_register_joint.load(std::memory_order_relaxed);
    }

private:
    static fci_arm_encode_scratch_t s_scratch(
        std::array<std::uint8_t, 256>& s_buffer) noexcept {
        return {s_buffer.data(), s_buffer.size()};
    }

    static wl_sink_result_t s_sink(void* s_user_data, wl_io_token_t,
                                   const std::uint8_t* s_data,
                                   std::size_t s_size) noexcept {
        auto& s_self = *static_cast<DevicePeer*>(s_user_data);
        return s_self.m_transport != nullptr &&
                       s_self.m_transport->deliver(s_data, s_size)
                   ? WL_SINK_SENT
                   : WL_SINK_FAILED;
    }

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
            case GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID:
                s_self.s_deviceSettings(s_context, s_event);
                break;
            case SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_device_settings_request_t,
                                        set_device_settings_response_t>(
                    s_context, s_event, set_device_settings_request_decode,
                    set_device_settings_response_clear,
                    fci_arm_set_device_settings_response_send_reliable,
                    DEVICE_SETTINGS_OK);
                break;
            case SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_arm_control_mode_request_t,
                                        set_arm_control_mode_response_t>(
                    s_context, s_event, set_arm_control_mode_request_decode,
                    set_arm_control_mode_response_clear,
                    fci_arm_set_arm_control_mode_response_send_reliable,
                    MODE_OK);
                break;
            case SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_gripper_control_mode_request_t,
                                        set_gripper_control_mode_response_t>(
                    s_context, s_event,
                    set_gripper_control_mode_request_decode,
                    set_gripper_control_mode_response_clear,
                    fci_arm_set_gripper_control_mode_response_send_reliable,
                    MODE_OK);
                break;
            case SET_ARM_MODE_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_arm_mode_request_t,
                                        set_arm_mode_response_t>(
                    s_context, s_event, set_arm_mode_request_decode,
                    set_arm_mode_response_clear,
                    fci_arm_set_arm_mode_response_send_reliable, MODE_OK);
                break;
            case HOME_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<home_request_t, home_response_t>(
                    s_context, s_event, home_request_decode,
                    home_response_clear,
                    fci_arm_home_response_send_reliable, HOME_OK);
                break;
            case CLEAR_FAULTS_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<clear_faults_request_t,
                                        clear_faults_response_t>(
                    s_context, s_event, clear_faults_request_decode,
                    clear_faults_response_clear,
                    fci_arm_clear_faults_response_send_reliable,
                    FAULT_OPERATION_OK);
                break;
            case CLEAR_ERROR_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<clear_error_request_t,
                                        clear_error_response_t>(
                    s_context, s_event, clear_error_request_decode,
                    clear_error_response_clear,
                    fci_arm_clear_error_response_send_reliable,
                    FAULT_OPERATION_OK);
                break;
            case MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID:
                s_self.s_motorRead(s_context, s_event);
                break;
            case MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<motor_register_write_request_t,
                                        motor_register_write_response_t>(
                    s_context, s_event,
                    motor_register_write_request_decode,
                    motor_register_write_response_clear,
                    fci_arm_motor_register_write_response_send_reliable,
                    MOTOR_OPERATION_OK);
                break;
            case MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<
                    motor_store_parameters_request_t,
                    motor_store_parameters_response_t>(
                    s_context, s_event,
                    motor_store_parameters_request_decode,
                    motor_store_parameters_response_clear,
                    fci_arm_motor_store_parameters_response_send_reliable,
                    MOTOR_OPERATION_OK);
                break;
            case MOTOR_SET_ZERO_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<motor_set_zero_request_t,
                                        motor_set_zero_response_t>(
                    s_context, s_event, motor_set_zero_request_decode,
                    motor_set_zero_response_clear,
                    fci_arm_motor_set_zero_response_send_reliable,
                    MOTOR_OPERATION_OK);
                break;
            default:
                s_self.s_recordCommand(s_event);
                break;
        }
        wl_event_release(&s_context, &s_event);
    }

    template <typename Request, typename Response, typename Decode,
              typename Clear, typename Send>
    void s_statusResponse(wl_ctx_t& s_context, const wl_event_t& s_event,
                          Decode s_decode, Clear s_clear, Send s_send,
                          std::int32_t s_status) noexcept {
        Request s_request{};
        if (s_decode(s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id) {
            return;
        }
        Response s_response{};
        s_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = s_status;
        std::array<std::uint8_t, 256> s_encode{};
        (void)s_send(&s_context, &s_response, s_scratch(s_encode));
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
        m_acquire_count.fetch_add(1, std::memory_order_relaxed);
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
        (void)fci_arm_acquire_control_lease_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode));
    }

    void s_release(wl_ctx_t& s_context,
                   const wl_event_t& s_event) noexcept {
        release_control_lease_request_t s_request{};
        if (release_control_lease_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id) {
            return;
        }
        m_release_count.fetch_add(1, std::memory_order_relaxed);
        release_control_lease_response_t s_response{};
        release_control_lease_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = CONTROL_LEASE_OK;
        std::array<std::uint8_t, 256> s_encode{};
        (void)fci_arm_release_control_lease_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode));
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
        constexpr char s_board[] = "loopback";
        constexpr char s_name[] = "test-arm";
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
        s_info.protocol_version.minor = 0;
        s_info.protocol_version.has_patch = true;
        s_info.protocol_version.patch = 0;
        s_info.has_firmware_version = true;
        s_info.firmware_version.has_major = true;
        s_info.firmware_version.major = 2;
        s_info.firmware_version.has_minor = true;
        s_info.firmware_version.minor = 3;
        s_info.firmware_version.has_patch = true;
        s_info.firmware_version.patch = 4;
        s_info.has_board_name = true;
        s_info.board_name = {s_board, sizeof(s_board) - 1};
        s_info.has_custom_name = true;
        s_info.custom_name = {s_name, sizeof(s_name) - 1};
        s_info.has_firmware_type = true;
        s_info.firmware_type = FIRMWARE_STANDARD_ARM;
        std::array<std::uint8_t, 256> s_encode{};
        (void)fci_arm_get_device_info_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode));
    }

    void s_deviceSettings(wl_ctx_t& s_context,
                          const wl_event_t& s_event) noexcept {
        get_device_settings_request_t s_request{};
        if (get_device_settings_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id) {
            return;
        }
        get_device_settings_response_t s_response{};
        get_device_settings_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = DEVICE_SETTINGS_OK;
        s_response.has_settings = true;
        auto& s_settings = s_response.settings;
        s_settings.has_firmware_dt_us = true;
        s_settings.firmware_dt_us = 1000;
        s_settings.has_gravity_scale = true;
        s_settings.has_torque_continuous = true;
        s_settings.has_torque_peak = true;
        s_settings.has_thermal_capacity = true;
        s_settings.has_torque_ramp_rate = true;
        s_settings.has_joint_limit_min = true;
        s_settings.has_joint_limit_max = true;
        for (std::size_t s_index = 0; s_index < 7; ++s_index) {
            s_settings.torque_continuous[s_index] = 1.0F;
            s_settings.torque_peak[s_index] = 2.0F;
            s_settings.thermal_capacity[s_index] = 3.0F;
            s_settings.torque_ramp_rate[s_index] = 4.0F;
        }
        for (std::size_t s_index = 0; s_index < 6; ++s_index) {
            s_settings.gravity_scale[s_index] = 1.0F;
            s_settings.joint_limit_min[s_index] = -2.0F;
            s_settings.joint_limit_max[s_index] = 2.0F;
        }
        std::array<std::uint8_t, 256> s_encode{};
        (void)fci_arm_get_device_settings_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode));
    }

    void s_motorRead(wl_ctx_t& s_context,
                     const wl_event_t& s_event) noexcept {
        motor_register_read_request_t s_request{};
        if (motor_register_read_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id || !s_request.has_joint_id ||
            !s_request.has_register_id) {
            return;
        }
        m_last_register_joint.store(s_request.joint_id,
                                    std::memory_order_relaxed);
        motor_register_read_response_t s_response{};
        motor_register_read_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = MOTOR_OPERATION_OK;
        s_response.has_joint_id = true;
        s_response.joint_id = s_request.joint_id;
        s_response.has_register_id = true;
        s_response.register_id = s_request.register_id;
        s_response.has_value = true;
        s_response.value = 12.5F;
        std::array<std::uint8_t, 256> s_encode{};
        (void)fci_arm_motor_register_read_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode));
    }

    void s_recordCommand(const wl_event_t& s_event) noexcept {
        bool s_valid = false;
        if (s_event.message_id == JOINT_MIT_COMMAND_MESSAGE_ID) {
            joint_mit_command_t s_command{};
            s_valid = joint_mit_command_decode(
                          s_event.payload, s_event.payload_len, &s_command) ==
                          WL_CODEC_OK &&
                      s_command.has_lease_token &&
                      s_command.lease_token == m_lease_token;
        } else if (s_event.message_id ==
                   GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID) {
            gripper_position_velocity_command_t s_command{};
            s_valid = gripper_position_velocity_command_decode(
                          s_event.payload, s_event.payload_len, &s_command) ==
                          WL_CODEC_OK &&
                      s_command.has_lease_token &&
                      s_command.lease_token == m_lease_token;
        }
        if (!s_valid) return;
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            m_commands.push_back(s_event.message_id);
        }
        m_changed.notify_all();
    }

    static constexpr std::uint64_t m_lease_token =
        UINT64_C(0x1020304050607080);
    PeerStorage m_storage;
    WirelinkExecutor m_executor;
    LoopbackTransport* m_transport{};
    mutable std::mutex m_mutex;
    std::condition_variable m_changed;
    std::vector<std::uint16_t> m_commands;
    std::atomic<std::size_t> m_acquire_count{};
    std::atomic<std::size_t> m_release_count{};
    std::atomic<std::uint8_t> m_last_register_joint{0xff};
};

bool LoopbackTransport::send(const std::uint8_t* s_data,
                             std::size_t s_size) {
    return m_peer.feed(s_data, s_size);
}

void testArmImplWirelinkPipeline() {
    DevicePeer s_peer;
    require(s_peer.initialize() == WL_OK, "peer initialization failed");
    auto s_transport = std::make_unique<LoopbackTransport>(s_peer);
    require(s_peer.attach(*s_transport) == WL_OK, "peer sink failed");
    require(s_peer.start() == WL_OK, "peer start failed");

    {
        ArmImpl s_impl(std::move(s_transport));
        require(s_impl.isConnected(), "ArmImpl did not complete handshake");
        require(s_peer.acquireCount() >= 1, "lease request was not observed");
        require(s_impl.getDeviceInfo().m_board_name == "loopback" &&
                    s_impl.getDeviceInfo().m_firmware_version.m_patch == 4,
                "typed device information was not cached");
        require(s_impl.firmwarePeriodUs() == 1000,
                "typed device settings were not cached");

        require(s_peer.sendArmStatus(42, 1.5F) == WL_OK,
                "peer telemetry submit failed");
        florid::ArmState s_state{};
        const auto s_state_deadline =
            std::chrono::steady_clock::now() + 2s;
        while (std::chrono::steady_clock::now() < s_state_deadline &&
               s_state.m_seq != 42) {
            s_state = s_impl.readOnce();
            if (s_state.m_seq != 42) std::this_thread::sleep_for(1ms);
        }
        require(s_state.m_seq == 42 && s_state.m_q[0] == 1.5F &&
                    s_state.m_gripper_q == 0.25F,
                "borrowed ArmStatus was not copied into a stable snapshot");

        s_impl.enable();
        s_impl.drag();
        s_impl.disable();
        s_impl.home();

        DeviceSettings s_settings = s_impl.getDeviceSettings();
        s_settings.m_firmware_period_us = 2000;
        require(s_impl.setDeviceSettings(s_settings),
                "SetDeviceSettings failed");
        require(s_impl.firmwarePeriodUs() == 2000,
                "settings cache was not updated after RPC success");

        const auto s_register =
            s_impl.readMotorRegister(3, MotorRegister::GearEfficiency);
        require(s_register && *s_register == 12.5F,
                "MotorRegisterRead typed result failed");
        require(s_peer.lastRegisterJoint() == 2,
                "public one-based joint ID was not bridged to wire zero-based ID");
        require(s_impl.writeMotorRegister(
                    3, MotorRegister::GearEfficiency, 7.5F) &&
                    s_impl.storeParameters(3) && s_impl.setZeroPoint(3),
                "motor mutation RPC failed");

        s_impl.automaticErrorRecovery();

        s_impl.s_prepareControl<JointMIT>();
        JointMIT s_joint{};
        s_joint.m_q[0] = 2.0F;
        s_joint.m_kp[0] = 10.0F;
        s_impl.s_sendCommand(s_joint);
        require(s_peer.waitForCommand(JOINT_MIT_COMMAND_MESSAGE_ID),
                "lease-bearing JointMIT did not reach the peer");

        s_impl.s_prepareGripperControl<JointPosVel>();
        JointPosVel s_gripper{};
        s_gripper.m_q[0] = 0.4F;
        s_gripper.m_dq[0] = 0.2F;
        s_impl.s_sendGripperCommand(s_gripper);
        require(s_peer.waitForCommand(
                    GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID),
                "lease-bearing gripper command did not reach the peer");
    }

    require(s_peer.releaseCount() == 1,
            "ArmImpl shutdown did not release the lease exactly once");
    s_peer.stop();
}

} // namespace

int main() {
    try {
        testArmImplWirelinkPipeline();
        return 0;
    } catch (const std::exception& s_error) {
        std::fprintf(stderr, "test_transport_pipeline: %s\n", s_error.what());
        return 1;
    }
}
