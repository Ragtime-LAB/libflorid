#include <stddef.h>
#include <stdint.h>

#include "fci_arm.h"

#if !SET_ZERO_REQUEST_HAS_MAX_ENCODED_SIZE || \
    !JOINT_VELOCITY_COMMAND_HAS_MAX_ENCODED_SIZE || \
    !JOINT_MIT_COMMAND_HAS_MAX_ENCODED_SIZE || \
    !ARM_STATUS_HAS_MAX_ENCODED_SIZE || !MOTOR_FEEDBACK_HAS_MAX_ENCODED_SIZE || \
    !ARM_DIAGNOSTICS_HAS_MAX_ENCODED_SIZE || \
    !MOTOR_REGISTER_READ_REQUEST_HAS_MAX_ENCODED_SIZE || \
    !DEVICE_INFO_HAS_MAX_ENCODED_SIZE || \
    !SET_DEVICE_INFO_REQUEST_HAS_MAX_ENCODED_SIZE || \
    !SET_DEVICE_SETTINGS_RESPONSE_HAS_MAX_ENCODED_SIZE
#error "fixed FCI messages must advertise static encoded-size bounds"
#endif

_Static_assert(SET_ZERO_REQUEST_MAX_ENCODED_SIZE == UINT64_C(3),
               "SetZeroRequest static maximum changed");
_Static_assert(JOINT_VELOCITY_COMMAND_MAX_ENCODED_SIZE == UINT64_C(52),
               "JointVelocityCommand static maximum changed");
_Static_assert(JOINT_MIT_COMMAND_MAX_ENCODED_SIZE == UINT64_C(160),
               "JointMitCommand static maximum changed");
_Static_assert(ARM_STATUS_MAX_ENCODED_SIZE == UINT64_C(233),
               "ArmStatus static maximum changed");
_Static_assert(MOTOR_FEEDBACK_MAX_ENCODED_SIZE == UINT64_C(128),
               "MotorFeedback static maximum changed");
_Static_assert(ARM_DIAGNOSTICS_MAX_ENCODED_SIZE == UINT64_C(67),
               "ArmDiagnostics static maximum changed");
_Static_assert(MOTOR_REGISTER_READ_REQUEST_MAX_ENCODED_SIZE == UINT64_C(6),
               "MotorRegisterReadRequest static maximum changed");
_Static_assert(DEVICE_INFO_MAX_ENCODED_SIZE == UINT64_C(154),
               "DeviceInfo static maximum changed");
_Static_assert(SET_DEVICE_INFO_REQUEST_MAX_ENCODED_SIZE == UINT64_C(33),
               "SetDeviceInfoRequest static maximum changed");
_Static_assert(SET_DEVICE_SETTINGS_RESPONSE_MAX_ENCODED_SIZE == UINT64_C(206),
               "SetDeviceSettingsResponse static maximum changed");
_Static_assert(sizeof(((set_zero_request_t *)0)->joint_id) == sizeof(uint8_t),
               "joint IDs must use uint8_t storage");
_Static_assert(sizeof(((motor_register_read_request_t *)0)->register_id) ==
                   sizeof(uint8_t),
               "register IDs must use uint8_t storage");
_Static_assert(sizeof(((motor_feedback_t *)0)->enabled_mask) == sizeof(uint8_t),
               "motor masks must use uint8_t storage");
_Static_assert(sizeof(((arm_diagnostics_t *)0)->bus_state) == sizeof(uint8_t),
               "bus state must use uint8_t storage");
_Static_assert(sizeof(((arm_diagnostics_t *)0)->tx_error_count) ==
                   sizeof(uint16_t),
               "bus error counters must use uint16_t storage");
_Static_assert(sizeof(((arm_diagnostics_t *)0)->rx_error_count) ==
                   sizeof(uint16_t),
               "bus error counters must use uint16_t storage");
_Static_assert(sizeof(((device_info_t *)0)->board_name) ==
                   sizeof(wl_codec_string_t),
               "bounded strings must remain borrowed views");
_Static_assert(sizeof(((device_info_t *)0)->serial) ==
                   sizeof(wl_codec_string_t),
               "device serials must remain borrowed views");
_Static_assert(sizeof(((joint_velocity_command_t *)0)->sequence) ==
                   sizeof(uint32_t),
               "command sequences must remain uint32_t");
_Static_assert(sizeof(((arm_status_t *)0)->timestamp_us) == sizeof(uint64_t),
               "timestamps must remain uint64_t");
_Static_assert(sizeof(((arm_status_t *)0)->error_flags) == sizeof(uint32_t),
               "error bitfields must remain uint32_t");

static int test_required_rpc(void)
{
    static const uint8_t joint_id_overflow[] = {0x10U, 0xacU, 0x02U};
    uint8_t encoded[SET_ZERO_REQUEST_MAX_ENCODED_SIZE];
    size_t length = 77U;
    set_zero_request_t request = {0}, decoded = {0};
    if (set_zero_request_encoded_size(&request) != SIZE_MAX ||
        set_zero_request_encode(&request, encoded, sizeof(encoded), &length) !=
            WL_CODEC_ERR_MISSING_REQUIRED_FIELD || length != 77U) return 1;
    request.has_joint_id = true;
    request.joint_id = 6U;
    if (set_zero_request_encode(&request, encoded, sizeof(encoded), &length) != WL_CODEC_OK ||
        length != 2U) return 2;
    if (set_zero_request_decode(encoded, length, &decoded) != WL_CODEC_OK ||
        !decoded.has_joint_id || decoded.joint_id != 6U) return 3;
    if (set_zero_request_decode(NULL, 0U, &decoded) != WL_CODEC_ERR_MISSING_REQUIRED_FIELD) return 4;
    if (set_zero_request_decode(encoded, length - 1U, &decoded) != WL_CODEC_ERR_MALFORMED) return 5;
    if (set_zero_request_decode(joint_id_overflow, sizeof(joint_id_overflow), &decoded) !=
        WL_CODEC_ERR_OVERFLOW) return 6;
    return 0;
}

static int test_required_realtime_vector(void)
{
    static const uint8_t enabled_mask_overflow[] = {
        0x10U, 0x80U, 0x02U,
    };
    uint8_t encoded[JOINT_VELOCITY_COMMAND_MAX_ENCODED_SIZE];
    size_t length = 91U;
    joint_velocity_command_t command = {0};
    joint_velocity_command_t decoded = {0};

    command.has_enabled_mask = true;
    command.enabled_mask = 0x3fU;
    command.has_sequence = true;
    command.sequence = 9U;
    command.has_sdk_timestamp_us = true;
    command.sdk_timestamp_us = UINT64_C(10000);
    command.has_lease_token = true;
    command.lease_token = UINT64_C(0x0123456789abcdef);

    if (joint_velocity_command_encoded_size(&command) != SIZE_MAX) {
        return 1;
    }
    if (joint_velocity_command_encode(&command, encoded, sizeof(encoded),
                                      &length) !=
            WL_CODEC_ERR_MISSING_REQUIRED_FIELD ||
        length != 91U) {
        return 2;
    }

    command.has_velocity = true;
    if (joint_velocity_command_encoded_size(&command) !=
        JOINT_VELOCITY_COMMAND_MAX_ENCODED_SIZE - UINT64_C(1)) {
        return 3;
    }
    if (joint_velocity_command_encode(&command, encoded, sizeof(encoded),
                                      &length) != WL_CODEC_OK ||
        length != JOINT_VELOCITY_COMMAND_MAX_ENCODED_SIZE - UINT64_C(1)) {
        return 4;
    }
    if (joint_velocity_command_decode(encoded, length, &decoded) != WL_CODEC_OK ||
        !decoded.has_velocity || !decoded.has_enabled_mask ||
        !decoded.has_sequence || !decoded.has_sdk_timestamp_us ||
        !decoded.has_lease_token) {
        return 5;
    }

    /* Field 1 is tag + 24-byte packed length + payload: 26 bytes total. */
    if (joint_velocity_command_decode(encoded + 26U, length - 26U, &decoded) !=
        WL_CODEC_ERR_MISSING_REQUIRED_FIELD) {
        return 6;
    }
    /* The final field is fixed64 lease_token; truncating it is malformed. */
    if (joint_velocity_command_decode(encoded, length - 1U, &decoded) !=
        WL_CODEC_ERR_MALFORMED) {
        return 7;
    }
    if (joint_velocity_command_decode(enabled_mask_overflow,
                                      sizeof(enabled_mask_overflow), &decoded) !=
        WL_CODEC_ERR_OVERFLOW) {
        return 8;
    }

    return 0;
}

static int test_bounded_device_name(void)
{
    static const char too_long[] = "01234567890123456789012345678901";
    static const uint8_t overbound_wire[] = {
        0x12U, 0x20U,
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '0', '1',
    };
    uint8_t encoded[SET_DEVICE_INFO_REQUEST_MAX_ENCODED_SIZE];
    size_t length = 55U;
    set_device_info_request_t request = {0};
    set_device_info_request_t decoded = {0};

    _Static_assert(sizeof(too_long) - 1U == 32U,
                   "test value must exceed the device-name bound by one byte");
    request.has_custom_name = true;
    request.custom_name =
        (wl_codec_string_t){too_long, sizeof(too_long) - 1U};
    if (set_device_info_request_encoded_size(&request) != SIZE_MAX) {
        return 1;
    }
    if (set_device_info_request_encode(&request, encoded, sizeof(encoded),
                                       &length) != WL_CODEC_ERR_INVALID_VALUE ||
        length != 55U) {
        return 2;
    }
    if (set_device_info_request_decode(overbound_wire,
                                       sizeof(overbound_wire), &decoded) !=
        WL_CODEC_ERR_INVALID_VALUE) {
        return 3;
    }

    return 0;
}

static int test_required_device_serial(void)
{
    static const char board_name[] = "willow";
    static const char custom_name[] = "lab-arm";
    static const char serial[] = "323738373233511200260036";
    static const char too_long[] = "01234567890123456789012345678901";
    uint8_t encoded[DEVICE_INFO_MAX_ENCODED_SIZE];
    size_t length = 99U;
    device_info_t info = {0};
    device_info_t decoded = {0};

    info.has_protocol_version = true;
    info.protocol_version.has_major = true;
    info.protocol_version.major = 1U;
    info.protocol_version.has_minor = true;
    info.protocol_version.minor = 0U;
    info.protocol_version.has_patch = true;
    info.protocol_version.patch = 0U;
    info.has_firmware_version = true;
    info.firmware_version.has_major = true;
    info.firmware_version.major = 1U;
    info.firmware_version.has_minor = true;
    info.firmware_version.minor = 0U;
    info.firmware_version.has_patch = true;
    info.firmware_version.patch = 0U;
    info.has_board_name = true;
    info.board_name =
        (wl_codec_string_t){board_name, sizeof(board_name) - 1U};
    info.has_custom_name = true;
    info.custom_name =
        (wl_codec_string_t){custom_name, sizeof(custom_name) - 1U};
    info.has_firmware_type = true;
    info.firmware_type = FIRMWARE_STANDARD_ARM;

    if (device_info_encoded_size(&info) != SIZE_MAX ||
        device_info_encode(&info, encoded, sizeof(encoded), &length) !=
            WL_CODEC_ERR_MISSING_REQUIRED_FIELD ||
        length != 99U) {
        return 1;
    }

    info.has_serial = true;
    info.serial = (wl_codec_string_t){serial, sizeof(serial) - 1U};
    if (device_info_encode(&info, encoded, sizeof(encoded), &length) !=
            WL_CODEC_OK ||
        device_info_decode(encoded, length, &decoded) != WL_CODEC_OK ||
        !decoded.has_serial || decoded.serial.length != sizeof(serial) - 1U) {
        return 2;
    }

    info.serial = (wl_codec_string_t){too_long, sizeof(too_long) - 1U};
    if (device_info_encoded_size(&info) != SIZE_MAX) {
        return 3;
    }

    return 0;
}

static void populate_settings(device_settings_t *settings)
{
    settings->has_firmware_dt_us = true;
    settings->firmware_dt_us = 2000U;
    settings->has_gravity_scale = true;
    settings->gravity_scale[0] = 1.0F;
    settings->has_torque_continuous = true;
    settings->torque_continuous[0] = 2.0F;
    settings->has_torque_peak = true;
    settings->torque_peak[0] = 3.0F;
    settings->has_thermal_capacity = true;
    settings->thermal_capacity[0] = 4.0F;
    settings->has_torque_ramp_rate = true;
    settings->torque_ramp_rate[0] = 5.0F;
    settings->has_joint_limit_min = true;
    settings->joint_limit_min[0] = -1.0F;
    settings->has_joint_limit_max = true;
    settings->joint_limit_max[0] = 1.0F;
}

static int test_transactional_settings_response(void)
{
    uint8_t encoded[SET_DEVICE_SETTINGS_RESPONSE_MAX_ENCODED_SIZE];
    size_t length = 0U;
    set_device_settings_response_t response = {0};
    set_device_settings_response_t decoded = {0};

    response.has_settings = true;
    populate_settings(&response.settings);

    if (set_device_settings_response_encode(&response, encoded,
                                            sizeof(encoded), &length) !=
            WL_CODEC_OK ||
        set_device_settings_response_decode(encoded, length, &decoded) !=
            WL_CODEC_OK ||
        !decoded.has_settings || !decoded.settings.has_firmware_dt_us ||
        decoded.settings.firmware_dt_us != 2000U ||
        decoded.settings.gravity_scale[0] != 1.0F ||
        decoded.settings.joint_limit_min[0] != -1.0F ||
        decoded.settings.joint_limit_max[0] != 1.0F) {
        return 1;
    }

    return 0;
}

int main(void)
{
    const int rpc_result = test_required_rpc();
    const int realtime_result = test_required_realtime_vector();
    const int bounded_name_result = test_bounded_device_name();
    const int serial_result = test_required_device_serial();
    const int settings_response_result =
        test_transactional_settings_response();

    if (rpc_result != 0) {
        return rpc_result;
    }
    if (realtime_result != 0) {
        return 20 + realtime_result;
    }
    if (bounded_name_result != 0) {
        return 40 + bounded_name_result;
    }
    if (serial_result != 0) {
        return 60 + serial_result;
    }
    if (settings_response_result != 0) {
        return 80 + settings_response_result;
    }
    return 0;
}
