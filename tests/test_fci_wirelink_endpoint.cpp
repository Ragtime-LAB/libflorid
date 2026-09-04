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
using florid::JointPVT;
using florid::JointPosVel;
using florid::JointVel;
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

class DirectTransportProbe final : public florid::Transport {
public:
    bool send(const std::uint8_t*, std::size_t) override { return false; }
    void setReceiveCallback(ReceiveFunctor, void*) override {}
    bool usesDirectWirelink() const noexcept override { return true; }

    int attachWirelink(wl_ctx_t& s_link, WakeFunctor s_wake,
                       void* s_context) noexcept override {
        m_wake = s_wake;
        m_wake_context = s_context;
        return wl_set_sink(&s_link, s_sink, this);
    }

    int serviceWirelink() noexcept override {
        m_service_calls.fetch_add(1, std::memory_order_relaxed);
        return WL_OK;
    }

    void quiesceWirelink() noexcept override {
        m_quiesced.store(true, std::memory_order_release);
    }

    void notify() noexcept {
        if (m_wake != nullptr) m_wake(m_wake_context);
    }

    std::atomic<std::uint64_t> m_service_calls{};
    std::atomic<bool> m_quiesced{};

private:
    static wl_sink_result_t s_sink(void*, wl_io_token_t, const std::uint8_t*,
                                   std::size_t) noexcept {
        return WL_SINK_SENT;
    }

    WakeFunctor m_wake{};
    void* m_wake_context{};
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
    std::size_t m_other_rpc_requests{};
    std::size_t m_joint_commands{};
    std::array<std::size_t, 9> m_control_messages{};
    joint_mit_command_t m_last_joint{};
    std::atomic<bool> m_drop_device_info{};
    std::atomic<std::uint64_t> m_command_capabilities{};
    // 0: OK with normalized settings, 1: malformed OK without settings,
    // 2: domain rejection without settings.
    std::atomic<int> m_set_settings_mode{};
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
            case SET_DEVICE_INFO_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_device_info_request_t,
                                        set_device_info_response_t>(
                    s_context, s_event, set_device_info_request_decode,
                    set_device_info_response_clear,
                    fci_arm_set_device_info_response_send_reliable,
                    DEVICE_INFO_OK);
                break;
            case GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID:
                s_self.s_deviceSettings(s_context, s_event);
                break;
            case SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID:
                s_self.s_setDeviceSettings(s_context, s_event);
                break;
            case SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_arm_control_mode_request_t,
                                        set_arm_control_mode_response_t>(
                    s_context, s_event,
                    set_arm_control_mode_request_decode,
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
            case SET_ZERO_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<set_zero_request_t,
                                        set_zero_response_t>(
                    s_context, s_event, set_zero_request_decode,
                    set_zero_response_clear,
                    fci_arm_set_zero_response_send_reliable,
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
            case CLEAR_FAULTS_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<clear_faults_request_t,
                                        clear_faults_response_t>(
                    s_context, s_event, clear_faults_request_decode,
                    clear_faults_response_clear,
                    fci_arm_clear_faults_response_send_reliable,
                    FAULT_OPERATION_OK);
                break;
            case EMERGENCY_STOP_REQUEST_MESSAGE_ID:
                s_self.s_statusResponse<emergency_stop_request_t,
                                        emergency_stop_response_t>(
                    s_context, s_event, emergency_stop_request_decode,
                    emergency_stop_response_clear,
                    fci_arm_emergency_stop_response_send_reliable,
                    EMERGENCY_STOP_OK);
                break;
            case MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID:
                s_self.s_motorRegisterRead(s_context, s_event);
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
            case JOINT_MIT_COMMAND_MESSAGE_ID:
                s_self.s_joint(s_event);
                break;
            case GRIPPER_MIT_COMMAND_MESSAGE_ID:
                s_self.s_control<gripper_mit_command_t>(
                    s_event, gripper_mit_command_decode, 0);
                break;
            case JOINT_POSITION_VELOCITY_COMMAND_MESSAGE_ID:
                s_self.s_control<joint_position_velocity_command_t>(
                    s_event, joint_position_velocity_command_decode, 1);
                break;
            case JOINT_VELOCITY_COMMAND_MESSAGE_ID:
                s_self.s_control<joint_velocity_command_t>(
                    s_event, joint_velocity_command_decode, 2);
                break;
            case JOINT_PVT_COMMAND_MESSAGE_ID:
                s_self.s_control<joint_pvt_command_t>(
                    s_event, joint_pvt_command_decode, 3);
                break;
            case CARTESIAN_POSE_COMMAND_MESSAGE_ID:
                s_self.s_control<cartesian_pose_command_t>(
                    s_event, cartesian_pose_command_decode, 4);
                break;
            case CARTESIAN_VELOCITY_COMMAND_MESSAGE_ID:
                s_self.s_control<cartesian_velocity_command_t>(
                    s_event, cartesian_velocity_command_decode, 5);
                break;
            case GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID:
                s_self.s_control<gripper_position_velocity_command_t>(
                    s_event, gripper_position_velocity_command_decode, 6);
                break;
            case GRIPPER_VELOCITY_COMMAND_MESSAGE_ID:
                s_self.s_control<gripper_velocity_command_t>(
                    s_event, gripper_velocity_command_decode, 7);
                break;
            case GRIPPER_PVT_COMMAND_MESSAGE_ID:
                s_self.s_control<gripper_pvt_command_t>(
                    s_event, gripper_pvt_command_decode, 8);
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
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_other_rpc_requests;
        }
        Response s_response{};
        s_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = s_status;
        std::array<std::uint8_t, 256> s_encode{};
        s_recordSend(s_send(&s_context, &s_response, s_scratch(s_encode)));
        m_cv.notify_all();
    }

    template <typename Message, typename Decode>
    void s_control(const wl_event_t& s_event, Decode s_decode,
                   std::size_t s_index) noexcept {
        Message s_message{};
        if (s_decode(s_event.payload, s_event.payload_len, &s_message) !=
                WL_CODEC_OK ||
            !s_message.has_lease_token ||
            s_message.lease_token != m_lease_token) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_control_messages[s_index];
        }
        m_cv.notify_all();
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
        constexpr char s_serial[] = "323738373233511200260036";
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
        s_info.has_serial = true;
        s_info.serial = {s_serial, sizeof(s_serial) - 1};
        s_info.has_command_capabilities = true;
        s_info.command_capabilities =
            m_command_capabilities.load(std::memory_order_acquire);
        std::array<std::uint8_t, 256> s_encode{};
        s_recordSend(fci_arm_get_device_info_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
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
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_other_rpc_requests;
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
        s_recordSend(fci_arm_get_device_settings_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
        m_cv.notify_all();
    }

    void s_setDeviceSettings(wl_ctx_t& s_context,
                             const wl_event_t& s_event) noexcept {
        set_device_settings_request_t s_request{};
        if (set_device_settings_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id || !s_request.has_settings) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_other_rpc_requests;
        }
        const int s_mode = m_set_settings_mode.load(std::memory_order_relaxed);
        set_device_settings_response_t s_response{};
        set_device_settings_response_clear(&s_response);
        s_response.has_operation_id = true;
        s_response.operation_id = s_request.operation_id;
        s_response.has_status = true;
        s_response.status = s_mode == 2 ? DEVICE_SETTINGS_INVALID_ARGUMENT
                                        : DEVICE_SETTINGS_OK;
        if (s_mode == 0) {
            s_response.has_settings = true;
            s_response.settings = s_request.settings;
            // Exercise host consumption of a device-normalized value.
            s_response.settings.firmware_dt_us = 1250;
        }
        std::array<std::uint8_t, 256> s_encode{};
        s_recordSend(fci_arm_set_device_settings_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
        m_cv.notify_all();
    }

    void s_motorRegisterRead(wl_ctx_t& s_context,
                             const wl_event_t& s_event) noexcept {
        motor_register_read_request_t s_request{};
        if (motor_register_read_request_decode(
                s_event.payload, s_event.payload_len, &s_request) !=
                WL_CODEC_OK ||
            !s_request.has_operation_id || !s_request.has_joint_id ||
            !s_request.has_register_id) {
            return;
        }
        {
            std::lock_guard<std::mutex> s_lock(m_mutex);
            ++m_other_rpc_requests;
        }
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
        s_recordSend(fci_arm_motor_register_read_response_send_reliable(
            &s_context, &s_response, s_scratch(s_encode)));
        m_cv.notify_all();
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
                s_info.m_firmware_type == florid::FirmwareType::kCobotArm &&
                s_info.m_serial_number == "323738373233511200260036" &&
                s_info.m_command_capabilities == 0U,
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

    auto s_expect_rpc = [&](const florid::detail::FciSubmitResult& s_submit,
                            const char* s_message) {
        require(s_submit.m_status == FciEndpointStatus::kOk, s_message);
        require(s_host.waitOperation(s_submit.m_request_id, 1s,
                                     s_operation) == FciEndpointStatus::kOk,
                s_message);
        require(s_host.takeOperation(s_submit.m_request_id, s_operation) ==
                    FciEndpointStatus::kOk,
                s_message);
    };

    s_expect_rpc(s_host.setDeviceInfo(
                     "\xE6\x9C\xBA\xE6\xA2\xB0\xE8\x87\x82", 250),
                 "SetDeviceInfo failed");
    const auto s_get_settings = s_host.getDeviceSettings(250);
    require(s_get_settings.m_status == FciEndpointStatus::kOk &&
                s_host.waitOperation(s_get_settings.m_request_id, 1s,
                                     s_operation) == FciEndpointStatus::kOk,
            "GetDeviceSettings failed");
    florid::DeviceSettings s_settings{};
    require(s_host.takeDeviceSettings(s_get_settings.m_request_id,
                                      s_operation, s_settings) ==
                FciEndpointStatus::kOk &&
                s_settings.m_firmware_period_us == 1000 &&
                s_settings.m_torque_fold[6].m_peak_torque == 2.0F,
            "GetDeviceSettings domain result failed");
    const auto s_set_settings = s_host.setDeviceSettings(s_settings, 250);
    require(s_set_settings.m_status == FciEndpointStatus::kOk &&
                s_host.waitOperation(s_set_settings.m_request_id, 1s,
                                     s_operation) == FciEndpointStatus::kOk,
            "SetDeviceSettings failed");
    florid::DeviceSettings s_effective{};
    require(s_host.takeDeviceSettings(s_set_settings.m_request_id,
                                      s_operation, s_effective) ==
                FciEndpointStatus::kOk &&
                s_effective.m_firmware_period_us == 1250,
            "SetDeviceSettings did not return device-normalized settings");

    s_device.m_set_settings_mode.store(1, std::memory_order_relaxed);
    const auto s_missing_settings = s_host.setDeviceSettings(s_settings, 250);
    require(s_missing_settings.m_status == FciEndpointStatus::kOk &&
                s_host.waitOperation(s_missing_settings.m_request_id, 1s,
                                     s_operation) ==
                    FciEndpointStatus::kInternalError,
            "OK response without settings was not a protocol error");
    florid::DeviceSettings s_unchanged{};
    s_unchanged.m_firmware_period_us = 777;
    require(s_host.takeDeviceSettings(s_missing_settings.m_request_id,
                                      s_operation, s_unchanged) ==
                FciEndpointStatus::kInternalError &&
                s_unchanged.m_firmware_period_us == 777,
            "malformed SetDeviceSettings exposed a value");

    s_device.m_set_settings_mode.store(2, std::memory_order_relaxed);
    const auto s_rejected_settings = s_host.setDeviceSettings(s_settings, 250);
    require(s_rejected_settings.m_status == FciEndpointStatus::kOk &&
                s_host.waitOperation(s_rejected_settings.m_request_id, 1s,
                                     s_operation) ==
                    FciEndpointStatus::kDomainError,
            "rejected settings did not retain the domain error");
    require(s_host.takeDeviceSettings(s_rejected_settings.m_request_id,
                                      s_operation, s_unchanged) ==
                FciEndpointStatus::kDomainError &&
                s_unchanged.m_firmware_period_us == 777,
            "rejected SetDeviceSettings exposed a value");
    s_device.m_set_settings_mode.store(0, std::memory_order_relaxed);

    auto s_equal_limits = s_settings;
    s_equal_limits.m_joint_limits[0].m_min = 0.5F;
    s_equal_limits.m_joint_limits[0].m_max = 0.5F;
    require(s_host.setDeviceSettings(s_equal_limits, 250).m_status ==
                FciEndpointStatus::kInvalidArgument,
            "equal joint limits were accepted");
    s_expect_rpc(s_host.setArmControlMode(
                     florid::detail::FciMotorControlMode::kMit, 250),
                 "SetArmControlMode failed");
    s_expect_rpc(s_host.setGripperControlMode(
                     florid::detail::FciMotorControlMode::kPvt, 250),
                 "SetGripperControlMode failed");
    s_expect_rpc(s_host.setArmMode(florid::detail::FciArmMode::kPc, 250),
                 "SetArmMode failed");
    s_expect_rpc(s_host.home(250), "Home failed");
    s_expect_rpc(s_host.setZero(2, 250), "SetZero failed");
    s_expect_rpc(s_host.clearError(2, 250), "ClearError failed");
    s_expect_rpc(s_host.clearFaults(250), "ClearFaults failed");
    s_expect_rpc(s_host.emergencyStop(250), "EmergencyStop failed");
    const auto s_read_register = s_host.readMotorRegister(2, 0x1e, 250);
    require(s_read_register.m_status == FciEndpointStatus::kOk &&
                s_host.waitOperation(s_read_register.m_request_id, 1s,
                                     s_operation) == FciEndpointStatus::kOk,
            "MotorRegisterRead failed");
    float s_register_value{};
    require(s_host.takeMotorRegister(s_read_register.m_request_id,
                                     s_operation, s_register_value) ==
                FciEndpointStatus::kOk &&
                s_register_value == 12.5F,
            "MotorRegisterRead typed result failed");
    s_expect_rpc(s_host.writeMotorRegister(2, 0x1e, 7.5F, 250),
                 "MotorRegisterWrite failed");
    s_expect_rpc(s_host.storeMotorParameters(2, 250),
                 "MotorStoreParameters failed");
    s_expect_rpc(s_host.setMotorZero(2, 250), "MotorSetZero failed");
    require(s_device.m_other_rpc_requests == 17,
            "not every typed RPC reached the peer");

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

    auto s_wait_control = [&](std::size_t s_index, const char* s_message) {
        waitFor(s_device.m_cv, s_device.m_mutex,
                [&] { return s_device.m_control_messages[s_index] == 1; },
                s_message);
    };
    require(s_host.sendGripperMit(s_joint, 2000, 101) ==
                FciEndpointStatus::kOk,
            "GripperMIT send failed");
    s_wait_control(0, "GripperMIT did not reach the peer");
    JointPosVel s_pos_vel{};
    require(s_host.sendJointPositionVelocity(s_pos_vel, 102) ==
                FciEndpointStatus::kOk,
            "JointPositionVelocity send failed");
    s_wait_control(1, "JointPositionVelocity did not reach the peer");
    JointVel s_velocity{};
    require(s_host.sendJointVelocity(s_velocity, 103) ==
                FciEndpointStatus::kOk,
            "JointVelocity send failed");
    s_wait_control(2, "JointVelocity did not reach the peer");
    JointPVT s_pvt{};
    require(s_host.sendJointPvt(s_pvt, 104) == FciEndpointStatus::kOk,
            "JointPVT send failed");
    s_wait_control(3, "JointPVT did not reach the peer");
    florid::CartesianPose s_pose{};
    require(s_host.sendCartesianPose(s_pose, 2000, 105) ==
                FciEndpointStatus::kUnsupported,
            "CartesianPose must fail closed until firmware advertises support");
    florid::CartesianVelocities s_twist{};
    require(s_host.sendCartesianVelocity(s_twist, 2000, 106) ==
                FciEndpointStatus::kUnsupported,
            "CartesianVelocity must fail closed until firmware advertises support");
    const auto s_cartesian_capabilities =
        florid::commandCapability(florid::CommandCapability::kCartesianPose) |
        florid::commandCapability(
            florid::CommandCapability::kCartesianVelocity);
    s_device.m_command_capabilities.store(s_cartesian_capabilities,
                                          std::memory_order_release);
    const auto s_capability_request = s_host.getDeviceInfo(250);
    require(s_capability_request.m_status == FciEndpointStatus::kOk &&
                s_host.waitOperation(s_capability_request.m_request_id, 1s,
                                     s_operation) == FciEndpointStatus::kOk &&
                s_host.takeDeviceInfo(s_capability_request.m_request_id,
                                      s_operation, s_info) ==
                    FciEndpointStatus::kOk &&
                s_info.m_command_capabilities == s_cartesian_capabilities,
            "command capability refresh failed");
    require(s_host.sendCartesianPose(s_pose, 2000, 105) ==
                FciEndpointStatus::kOk,
            "advertised CartesianPose was rejected");
    s_wait_control(4, "CartesianPose did not reach the peer");
    require(s_host.sendCartesianVelocity(s_twist, 2000, 106) ==
                FciEndpointStatus::kOk,
            "advertised CartesianVelocity was rejected");
    s_wait_control(5, "CartesianVelocity did not reach the peer");
    require(s_host.sendGripperPositionVelocity(s_pos_vel, 107) ==
                FciEndpointStatus::kOk,
            "GripperPositionVelocity send failed");
    s_wait_control(6, "GripperPositionVelocity did not reach the peer");
    require(s_host.sendGripperVelocity(s_velocity, 108) ==
                FciEndpointStatus::kOk,
            "GripperVelocity send failed");
    s_wait_control(7, "GripperVelocity did not reach the peer");
    require(s_host.sendGripperPvt(s_pvt, 109) == FciEndpointStatus::kOk,
            "GripperPVT send failed");
    s_wait_control(8, "GripperPVT did not reach the peer");

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
    // One value may already be staged in the core while the sink reports
    // BUSY; the remaining fixed lane must still coalesce at least 17 of the
    // 19 replacements regardless of owner-thread scheduling.
    require(s_host.executorStats().m_latest_coalesced >= 17,
            "LATEST replacement was not recorded");

    require(s_device.sendArmStatus(41, 4.25F) == WL_OK,
            "ArmStatus submit failed");
    waitFor(s_status_capture.m_cv, s_status_capture.m_mutex,
            [&] { return s_status_capture.m_calls == 1; },
            "ArmStatus callback did not run");
    {
        std::lock_guard<std::mutex> s_lock(s_status_capture.m_mutex);
        require(s_status_capture.m_status.m_state.m_seq == 41 &&
                    s_status_capture.m_status.m_state.m_q[0] == 4.25F,
                "ArmStatus callback snapshot was not stable after borrowed release");
    }

    // A direct asynchronous transport can still own the preceding ACK when
    // the next RPC is queued. Temporary sink backpressure must leave the
    // public request queued instead of exposing a terminal LINK_FAILED.
    const auto s_rpc_started_before_busy = s_host.stats().m_rpc_started;
    s_host_to_device.m_busy.store(true, std::memory_order_release);
    const auto s_busy_info = s_host.getDeviceInfo(250);
    require(s_busy_info.m_status == FciEndpointStatus::kOk,
            "backpressured GetDeviceInfo was not queued");
    const auto s_busy_start_deadline =
        std::chrono::steady_clock::now() + 1s;
    while (s_host.stats().m_rpc_started == s_rpc_started_before_busy &&
           std::chrono::steady_clock::now() < s_busy_start_deadline) {
        std::this_thread::yield();
    }
    require(s_host.stats().m_rpc_started > s_rpc_started_before_busy,
            "backpressured RPC never attempted a runtime send");
    s_host_to_device.m_busy.store(false, std::memory_order_release);
    s_host.notify();
    require(s_host.waitOperation(s_busy_info.m_request_id, 1s,
                                 s_operation) == FciEndpointStatus::kOk,
            "backpressured RPC did not retry after transport wake");
    require(s_host.takeDeviceInfo(s_busy_info.m_request_id, s_operation,
                                  s_info) == FciEndpointStatus::kOk,
            "retried GetDeviceInfo result was not consumable");

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
    // Let the terminal RX/TX wake already queued by the timeout settle before
    // measuring idle behavior. The second window must remain completely
    // event-driven.
    std::this_thread::sleep_for(50ms);
    const auto s_idle_poll_count = s_host.stats().m_runtime_poll_calls;
    std::this_thread::sleep_for(30ms);
    require(s_host.stats().m_runtime_poll_calls == s_idle_poll_count,
            "idle endpoint retained a hidden periodic wakeup");

    const auto s_rpc_started_before_shutdown =
        s_host.stats().m_rpc_started;
    s_host_to_device.m_busy.store(true, std::memory_order_release);
    const auto s_pending = s_host.getDeviceInfo(500);
    require(s_pending.m_status == FciEndpointStatus::kOk,
            "pending shutdown RPC was not queued");
    // A queued operation owns no generated-runtime slot yet.  Waiting an
    // arbitrary amount of wall time made this test depend on whether the
    // executor happened to run before stop(); wait for the slot acquisition
    // that the release/cancellation counters are intended to verify.
    const auto s_rpc_start_deadline = std::chrono::steady_clock::now() + 1s;
    while (s_host.stats().m_rpc_started == s_rpc_started_before_shutdown &&
           std::chrono::steady_clock::now() < s_rpc_start_deadline) {
        std::this_thread::yield();
    }
    require(s_host.stats().m_rpc_started ==
                s_rpc_started_before_shutdown + 1,
            "pending shutdown RPC never acquired a runtime slot");
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

void testDirectTransportLifecycle() {
    DirectTransportProbe s_transport;
    FciWirelinkEndpoint s_endpoint;
    FciWirelinkEndpointConfig s_config{};
    s_config.m_session_id = UINT64_C(0x445566778899aabb);
    require(s_endpoint.initialize(s_config) == FciEndpointStatus::kOk,
            "direct endpoint initialization failed");
    require(s_endpoint.attachDirectTransport(s_transport) ==
                FciEndpointStatus::kOk,
            "direct transport attach failed");
    require(s_endpoint.start() == FciEndpointStatus::kOk,
            "direct endpoint start failed");

    const auto s_deadline = std::chrono::steady_clock::now() + 1s;
    while (s_transport.m_service_calls.load(std::memory_order_acquire) == 0 &&
           std::chrono::steady_clock::now() < s_deadline) {
        std::this_thread::yield();
    }
    const auto s_before =
        s_transport.m_service_calls.load(std::memory_order_acquire);
    require(s_before != 0, "direct transport was not serviced by the owner");
    s_transport.notify();
    const auto s_wake_deadline = std::chrono::steady_clock::now() + 1s;
    while (s_transport.m_service_calls.load(std::memory_order_acquire) ==
               s_before &&
           std::chrono::steady_clock::now() < s_wake_deadline) {
        std::this_thread::yield();
    }
    require(s_transport.m_service_calls.load(std::memory_order_acquire) >
                s_before,
            "direct transport activity did not wake the owner");

    s_endpoint.stop();
    require(s_transport.m_quiesced.load(std::memory_order_acquire),
            "direct transport was not quiesced on the owner");
}

} // namespace

int main() {
    try {
        testTypedEndpointLifecycle();
        testDirectTransportLifecycle();
        std::puts("PASS: typed FCI endpoint owns runtime, RPC, LATEST, and shutdown");
        return 0;
    } catch (const std::exception& s_error) {
        std::fprintf(stderr, "FAIL: %s\n", s_error.what());
        return 1;
    }
}
