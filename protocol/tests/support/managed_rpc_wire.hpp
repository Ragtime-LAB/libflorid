// SPDX-License-Identifier: ISC
#pragma once

// Fault-injection peers only. These are managed-wire envelopes, NOT application
// DTOs or compatibility APIs. Explicit metadata lets tests resend an exact call,
// forge a conflicting identity, reject without a body, or restart a client.
// Production code must use the generated endpoint rather than this harness.
#if defined(FCI_TEST_DEVICE)
#include <fci_device_bindings.h>
using fci_arm_send_result_t = fci_device_send_result_t;
inline constexpr auto FCI_ARM_SEND_CODEC_ERROR = FCI_DEVICE_SEND_CODEC_ERROR;
inline constexpr auto FCI_ARM_SEND_CORE_ERROR = FCI_DEVICE_SEND_CORE_ERROR;
#else
#include <fci_arm_bindings.h>
#endif
#include <array>
#include <cstdint>

namespace test_rpc {

inline std::uint64_t default_session = 1U;

inline std::uint64_t read_be(const std::uint8_t *data, unsigned count) {
  std::uint64_t value = 0U;
  for (unsigned i = 0U; i < count; ++i) value = (value << 8U) | data[i];
  return value;
}
inline void write_be(std::uint8_t *data, std::uint64_t value, unsigned count) {
  for (unsigned i = 0U; i < count; ++i) data[count - i - 1U] = static_cast<std::uint8_t>(value >> (8U * i));
}

template <typename Body, bool Request, auto Encode, auto Decode>
struct Envelope : Body {
  bool has_operation_id{};
  std::uint32_t operation_id{};
  bool has_status{};
  std::int32_t status{};
  std::uint64_t session{default_session};

  static void clear(Envelope *out) { *out = {}; }
  static wl_codec_status_t encode(const Envelope *value, std::uint8_t *out,
                                  std::size_t capacity, std::size_t *size) {
    if (value == nullptr || out == nullptr || size == nullptr || capacity < 20U ||
        !value->has_operation_id || value->operation_id == 0U || value->session == 0U)
      return WL_CODEC_ERR_INVALID_VALUE;
    std::size_t body_size = 0U;
    if (Request || value->status == 0) {
      auto error = Encode(static_cast<const Body *>(value), out + 20U, capacity - 20U, &body_size);
      if (error != WL_CODEC_OK) return error;
    }
    out[0] = 0U; out[1] = 2U; out[2] = Request ? 1U : 2U; out[3] = 0U;
    write_be(out + 4U, value->operation_id, 4U);
    write_be(out + 8U, Request ? 0U : static_cast<std::uint32_t>(value->status), 4U);
    write_be(out + 12U, value->session, 8U);
    *size = body_size + 20U;
    return WL_CODEC_OK;
  }
  static wl_codec_status_t decode(const std::uint8_t *data, std::size_t size, Envelope *out) {
    if (data == nullptr || out == nullptr || size < 20U || data[0] != 0U || data[1] != 2U ||
        data[2] != (Request ? 1U : 2U) || data[3] != 0U)
      return WL_CODEC_ERR_INVALID_VALUE;
    *out = {};
    out->operation_id = static_cast<std::uint32_t>(read_be(data + 4U, 4U));
    out->status = static_cast<std::int32_t>(static_cast<std::uint32_t>(read_be(data + 8U, 4U)));
    out->session = read_be(data + 12U, 8U);
    out->has_operation_id = out->operation_id != 0U;
    out->has_status = !Request;
    if (out->session == 0U || !out->has_operation_id || (Request && out->status != 0))
      return WL_CODEC_ERR_INVALID_VALUE;
    if (!Request && out->status != 0)
      return size == 20U ? WL_CODEC_OK : WL_CODEC_ERR_INVALID_VALUE;
    return Decode(data + 20U, size - 20U, static_cast<Body *>(out));
  }
};

template <typename Message, std::uint16_t Id>
fci_arm_send_result_t send(wl_ctx_t *link, const Message *message,
                           wl_delivery_t delivery, wl_time_ms_t now) {
  std::array<std::uint8_t, 256> payload{};
  fci_arm_send_result_t result{};
  // Requests identify their actual test client, including after init_host().
  Message request = *message;
  result.codec_status = Message::encode(&request, payload.data(), payload.size(), &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_SEND_CODEC_ERROR;
    return result;
  }
  result.core_result = delivery == WL_DELIVERY_RELIABLE
      ? wl_send_reliable(link, Id, payload.data(), result.payload_length, now, &result.handle)
      : wl_send_unreliable(link, Id, payload.data(), result.payload_length);
  if (result.core_result != WL_OK) result.domain = FCI_ARM_SEND_CORE_ERROR;
  return result;
}

// Stable, explicit harness names keep raw test metadata out of business types.
#define FCI_TEST_ENVELOPE(name, is_request, id) \
  using name##_t = Envelope<::name##_t, is_request, ::name##_encode, ::name##_decode>; \
  inline constexpr auto name##_clear = name##_t::clear; \
  inline constexpr auto name##_encode = name##_t::encode; \
  inline constexpr auto name##_decode = name##_t::decode; \
  inline constexpr auto name##_send = send<name##_t, id>;

FCI_TEST_ENVELOPE(acquire_control_lease_request, true, ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(acquire_control_lease_response, false, ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(release_control_lease_request, true, RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(release_control_lease_response, false, RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(get_device_info_request, true, GET_DEVICE_INFO_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(get_device_info_response, false, GET_DEVICE_INFO_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_device_info_request, true, SET_DEVICE_INFO_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_device_info_response, false, SET_DEVICE_INFO_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(get_device_settings_request, true, GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(get_device_settings_response, false, GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_device_settings_request, true, SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_device_settings_response, false, SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_arm_control_mode_request, true, SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_arm_control_mode_response, false, SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_gripper_control_mode_request, true, SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_gripper_control_mode_response, false, SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_arm_mode_request, true, SET_ARM_MODE_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_arm_mode_response, false, SET_ARM_MODE_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(home_request, true, HOME_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(home_response, false, HOME_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_zero_request, true, SET_ZERO_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(set_zero_response, false, SET_ZERO_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(clear_error_request, true, CLEAR_ERROR_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(clear_error_response, false, CLEAR_ERROR_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(clear_faults_request, true, CLEAR_FAULTS_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(clear_faults_response, false, CLEAR_FAULTS_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(emergency_stop_request, true, EMERGENCY_STOP_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(emergency_stop_response, false, EMERGENCY_STOP_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_register_read_request, true, MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_register_read_response, false, MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_register_write_request, true, MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_register_write_response, false, MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_store_parameters_request, true, MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_store_parameters_response, false, MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_set_zero_request, true, MOTOR_SET_ZERO_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(motor_set_zero_response, false, MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID)
FCI_TEST_ENVELOPE(get_motor_feedback_request, true, GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID)
FCI_TEST_ENVELOPE(get_motor_feedback_response, false, GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID)

#undef FCI_TEST_ENVELOPE
} // namespace test_rpc
