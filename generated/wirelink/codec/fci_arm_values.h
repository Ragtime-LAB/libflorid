#ifndef WIRELINK_GENERATED_FCI_ARM_H_VALUES
#define WIRELINK_GENERATED_FCI_ARM_H_VALUES

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wirelink/codec.h>
#include <float.h>

#if defined(__cplusplus)
static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 && FLT_MIN_EXP == -125, "WLC float32 requires IEEE-754 binary32");
#else
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 && FLT_MIN_EXP == -125, "WLC float32 requires IEEE-754 binary32");
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t firmware_type_t;
#define FIRMWARE_STANDARD_ARM INT32_C(0)
#define FIRMWARE_MOBILE_ARM INT32_C(1)
#define FIRMWARE_COBOT_ARM INT32_C(2)

typedef int32_t arm_mode_t;
#define ARM_MODE_PC INT32_C(0)
#define ARM_MODE_DRAG INT32_C(1)
#define ARM_MODE_DAMP INT32_C(2)
#define ARM_MODE_RETRACTING INT32_C(3)
#define ARM_MODE_TELEOP INT32_C(4)

typedef int32_t motor_control_mode_t;
#define MOTOR_CONTROL_MIT INT32_C(1)
#define MOTOR_CONTROL_POSITION_VELOCITY INT32_C(2)
#define MOTOR_CONTROL_VELOCITY INT32_C(3)
#define MOTOR_CONTROL_PVT INT32_C(4)

typedef int32_t device_info_status_t;
#define DEVICE_INFO_OK INT32_C(0)
#define DEVICE_INFO_STORAGE_FAILED INT32_C(1)
#define DEVICE_INFO_NAME_TOO_LONG INT32_C(2)
#define DEVICE_INFO_INVALID_ARGUMENT INT32_C(3)
#define DEVICE_INFO_BUSY INT32_C(4)
#define DEVICE_INFO_INTERNAL_ERROR INT32_C(5)

typedef int32_t device_settings_status_t;
#define DEVICE_SETTINGS_OK INT32_C(0)
#define DEVICE_SETTINGS_STORAGE_FAILED INT32_C(1)
#define DEVICE_SETTINGS_INVALID_ARGUMENT INT32_C(2)
#define DEVICE_SETTINGS_BUSY INT32_C(3)
#define DEVICE_SETTINGS_INTERNAL_ERROR INT32_C(4)

typedef int32_t mode_status_t;
#define MODE_OK INT32_C(0)
#define MODE_INVALID_MODE INT32_C(1)
#define MODE_INVALID_STATE INT32_C(2)
#define MODE_BUSY INT32_C(3)
#define MODE_REJECTED INT32_C(4)
#define MODE_INTERNAL_ERROR INT32_C(5)

typedef int32_t home_status_t;
#define HOME_OK INT32_C(0)
#define HOME_BUSY INT32_C(1)
#define HOME_INVALID_STATE INT32_C(2)
#define HOME_FAILED INT32_C(3)
#define HOME_TIMED_OUT INT32_C(4)
#define HOME_CANCELLED INT32_C(5)

typedef int32_t motor_operation_status_t;
#define MOTOR_OPERATION_OK INT32_C(0)
#define MOTOR_OPERATION_INVALID_JOINT INT32_C(1)
#define MOTOR_OPERATION_INVALID_REGISTER INT32_C(2)
#define MOTOR_OPERATION_NOT_IN_DAMP_MODE INT32_C(3)
#define MOTOR_OPERATION_BUSY INT32_C(4)
#define MOTOR_OPERATION_TIMED_OUT INT32_C(5)
#define MOTOR_OPERATION_IO_ERROR INT32_C(6)
#define MOTOR_OPERATION_UNSUPPORTED INT32_C(7)

typedef int32_t fault_operation_status_t;
#define FAULT_OPERATION_OK INT32_C(0)
#define FAULT_OPERATION_INVALID_JOINT INT32_C(1)
#define FAULT_OPERATION_INVALID_STATE INT32_C(2)
#define FAULT_OPERATION_BUSY INT32_C(3)
#define FAULT_OPERATION_TIMED_OUT INT32_C(4)
#define FAULT_OPERATION_FAILED INT32_C(5)
#define FAULT_OPERATION_UNSUPPORTED INT32_C(6)

typedef int32_t emergency_stop_status_t;
#define EMERGENCY_STOP_OK INT32_C(0)
#define EMERGENCY_STOP_ALREADY_STOPPED INT32_C(1)
#define EMERGENCY_STOP_REJECTED INT32_C(2)
#define EMERGENCY_STOP_INTERNAL_ERROR INT32_C(3)

typedef int32_t control_lease_status_t;
#define CONTROL_LEASE_OK INT32_C(0)
#define CONTROL_LEASE_BUSY INT32_C(1)
#define CONTROL_LEASE_INVALID_TIMEOUT INT32_C(2)
#define CONTROL_LEASE_INVALID_TOKEN INT32_C(3)
#define CONTROL_LEASE_EXPIRED INT32_C(4)
#define CONTROL_LEASE_NOT_HELD INT32_C(5)
#define CONTROL_LEASE_INTERNAL_ERROR INT32_C(6)

/* Self-owning business values. Assignment copies all data; no destroy is needed.
 * String lengths are bytes (embedded NUL is allowed); data[length] is a
 * convenience terminator after clear/decode/from_view, not part of the wire.
 * Views borrow their source. Conversion input/output must not overlap.
 * Failed conversions/decodes leave output unchanged. */
#define SEMANTIC_VERSION_HAS_VALUE 1
typedef struct {
  bool has_major;
  uint32_t major;
  bool has_minor;
  uint32_t minor;
  bool has_patch;
  uint32_t patch;
} semantic_version_value_t;
#define SEMANTIC_VERSION_VALUE_SIZE (sizeof(semantic_version_value_t))
void semantic_version_value_clear(semantic_version_value_t *value);
size_t semantic_version_value_encoded_size(const semantic_version_value_t *value);
wl_codec_status_t semantic_version_value_encode(const semantic_version_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t semantic_version_value_decode(const uint8_t *input, size_t length, semantic_version_value_t *out);

#define DEVICE_INFO_HAS_VALUE 1
typedef struct {
  bool has_protocol_version;
  semantic_version_value_t protocol_version;
  bool has_firmware_version;
  semantic_version_value_t firmware_version;
  bool has_board_name;
  struct { size_t length; char data[32]; } board_name;
  bool has_custom_name;
  struct { size_t length; char data[32]; } custom_name;
  bool has_firmware_type;
  firmware_type_t firmware_type;
  bool has_serial;
  struct { size_t length; char data[32]; } serial;
  bool has_command_capabilities;
  uint64_t command_capabilities;
} device_info_value_t;
#define DEVICE_INFO_VALUE_SIZE (sizeof(device_info_value_t))
void device_info_value_clear(device_info_value_t *value);
size_t device_info_value_encoded_size(const device_info_value_t *value);
wl_codec_status_t device_info_value_encode(const device_info_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t device_info_value_decode(const uint8_t *input, size_t length, device_info_value_t *out);

#define DEVICE_SETTINGS_HAS_VALUE 1
typedef struct {
  bool has_firmware_dt_us;
  uint32_t firmware_dt_us;
  bool has_gravity_scale;
  float gravity_scale[6];
  bool has_torque_continuous;
  float torque_continuous[7];
  bool has_torque_peak;
  float torque_peak[7];
  bool has_thermal_capacity;
  float thermal_capacity[7];
  bool has_torque_ramp_rate;
  float torque_ramp_rate[7];
  bool has_joint_limit_min;
  float joint_limit_min[6];
  bool has_joint_limit_max;
  float joint_limit_max[6];
} device_settings_value_t;
#define DEVICE_SETTINGS_VALUE_SIZE (sizeof(device_settings_value_t))
void device_settings_value_clear(device_settings_value_t *value);
size_t device_settings_value_encoded_size(const device_settings_value_t *value);
wl_codec_status_t device_settings_value_encode(const device_settings_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t device_settings_value_decode(const uint8_t *input, size_t length, device_settings_value_t *out);

#define ARM_STATUS_HAS_VALUE 1
typedef struct {
  bool has_mode;
  arm_mode_t mode;
  bool has_sequence;
  uint32_t sequence;
  bool has_timestamp_us;
  uint64_t timestamp_us;
  bool has_joint_position;
  float joint_position[6];
  bool has_joint_velocity;
  float joint_velocity[6];
  bool has_joint_torque;
  float joint_torque[6];
  bool has_base_gravity;
  float base_gravity[3];
  bool has_gripper_position;
  float gripper_position;
  bool has_gripper_velocity;
  float gripper_velocity;
  bool has_gripper_torque;
  float gripper_torque;
  bool has_end_effector_transform;
  float end_effector_transform[16];
  bool has_external_wrench;
  float external_wrench[6];
  bool has_error_flags;
  uint32_t error_flags;
  bool has_last_sdk_timestamp_us;
  uint64_t last_sdk_timestamp_us;
} arm_status_value_t;
#define ARM_STATUS_VALUE_SIZE (sizeof(arm_status_value_t))
void arm_status_value_clear(arm_status_value_t *value);
size_t arm_status_value_encoded_size(const arm_status_value_t *value);
wl_codec_status_t arm_status_value_encode(const arm_status_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t arm_status_value_decode(const uint8_t *input, size_t length, arm_status_value_t *out);

#define MOTOR_FEEDBACK_HAS_VALUE 1
typedef struct {
  bool has_position_rad;
  float position_rad[7];
  bool has_velocity_rad_s;
  float velocity_rad_s[7];
  bool has_torque_nm;
  float torque_nm[7];
  bool has_temperature_c;
  float temperature_c[7];
  bool has_device_status_bits;
  uint32_t device_status_bits;
  bool has_enabled_mask;
  uint8_t enabled_mask;
} motor_feedback_value_t;
#define MOTOR_FEEDBACK_VALUE_SIZE (sizeof(motor_feedback_value_t))
void motor_feedback_value_clear(motor_feedback_value_t *value);
size_t motor_feedback_value_encoded_size(const motor_feedback_value_t *value);
wl_codec_status_t motor_feedback_value_encode(const motor_feedback_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_feedback_value_decode(const uint8_t *input, size_t length, motor_feedback_value_t *out);

#define ARM_DIAGNOSTICS_HAS_VALUE 1
typedef struct {
  bool has_uptime_s;
  uint32_t uptime_s;
  bool has_tick_count;
  uint32_t tick_count;
  bool has_mode_entry_ms;
  uint32_t mode_entry_ms;
  bool has_bus_healthy;
  bool bus_healthy;
  bool has_bus_state;
  uint8_t bus_state;
  bool has_tx_error_count;
  uint16_t tx_error_count;
  bool has_rx_error_count;
  uint16_t rx_error_count;
  bool has_joint_healthy_mask;
  uint8_t joint_healthy_mask;
  bool has_joint_temperature_c;
  float joint_temperature_c[6];
  bool has_gripper_healthy;
  bool gripper_healthy;
  bool has_gripper_temperature_c;
  float gripper_temperature_c;
  bool has_overheat_mask;
  uint8_t overheat_mask;
} arm_diagnostics_value_t;
#define ARM_DIAGNOSTICS_VALUE_SIZE (sizeof(arm_diagnostics_value_t))
void arm_diagnostics_value_clear(arm_diagnostics_value_t *value);
size_t arm_diagnostics_value_encoded_size(const arm_diagnostics_value_t *value);
wl_codec_status_t arm_diagnostics_value_encode(const arm_diagnostics_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t arm_diagnostics_value_decode(const uint8_t *input, size_t length, arm_diagnostics_value_t *out);

#define SET_ZERO_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
} set_zero_request_value_t;
#define SET_ZERO_REQUEST_VALUE_SIZE (sizeof(set_zero_request_value_t))
void set_zero_request_value_clear(set_zero_request_value_t *value);
size_t set_zero_request_value_encoded_size(const set_zero_request_value_t *value);
wl_codec_status_t set_zero_request_value_encode(const set_zero_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_zero_request_value_decode(const uint8_t *input, size_t length, set_zero_request_value_t *out);

#define SET_ZERO_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  fault_operation_status_t status;
} set_zero_response_value_t;
#define SET_ZERO_RESPONSE_VALUE_SIZE (sizeof(set_zero_response_value_t))
void set_zero_response_value_clear(set_zero_response_value_t *value);
size_t set_zero_response_value_encoded_size(const set_zero_response_value_t *value);
wl_codec_status_t set_zero_response_value_encode(const set_zero_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_zero_response_value_decode(const uint8_t *input, size_t length, set_zero_response_value_t *out);

#define CLEAR_ERROR_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
} clear_error_request_value_t;
#define CLEAR_ERROR_REQUEST_VALUE_SIZE (sizeof(clear_error_request_value_t))
void clear_error_request_value_clear(clear_error_request_value_t *value);
size_t clear_error_request_value_encoded_size(const clear_error_request_value_t *value);
wl_codec_status_t clear_error_request_value_encode(const clear_error_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t clear_error_request_value_decode(const uint8_t *input, size_t length, clear_error_request_value_t *out);

#define CLEAR_ERROR_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  fault_operation_status_t status;
} clear_error_response_value_t;
#define CLEAR_ERROR_RESPONSE_VALUE_SIZE (sizeof(clear_error_response_value_t))
void clear_error_response_value_clear(clear_error_response_value_t *value);
size_t clear_error_response_value_encoded_size(const clear_error_response_value_t *value);
wl_codec_status_t clear_error_response_value_encode(const clear_error_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t clear_error_response_value_decode(const uint8_t *input, size_t length, clear_error_response_value_t *out);

#define HOME_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
} home_request_value_t;
#define HOME_REQUEST_VALUE_SIZE (sizeof(home_request_value_t))
void home_request_value_clear(home_request_value_t *value);
size_t home_request_value_encoded_size(const home_request_value_t *value);
wl_codec_status_t home_request_value_encode(const home_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t home_request_value_decode(const uint8_t *input, size_t length, home_request_value_t *out);

#define HOME_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  home_status_t status;
} home_response_value_t;
#define HOME_RESPONSE_VALUE_SIZE (sizeof(home_response_value_t))
void home_response_value_clear(home_response_value_t *value);
size_t home_response_value_encoded_size(const home_response_value_t *value);
wl_codec_status_t home_response_value_encode(const home_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t home_response_value_decode(const uint8_t *input, size_t length, home_response_value_t *out);

#define CLEAR_FAULTS_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
} clear_faults_request_value_t;
#define CLEAR_FAULTS_REQUEST_VALUE_SIZE (sizeof(clear_faults_request_value_t))
void clear_faults_request_value_clear(clear_faults_request_value_t *value);
size_t clear_faults_request_value_encoded_size(const clear_faults_request_value_t *value);
wl_codec_status_t clear_faults_request_value_encode(const clear_faults_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t clear_faults_request_value_decode(const uint8_t *input, size_t length, clear_faults_request_value_t *out);

#define CLEAR_FAULTS_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  fault_operation_status_t status;
} clear_faults_response_value_t;
#define CLEAR_FAULTS_RESPONSE_VALUE_SIZE (sizeof(clear_faults_response_value_t))
void clear_faults_response_value_clear(clear_faults_response_value_t *value);
size_t clear_faults_response_value_encoded_size(const clear_faults_response_value_t *value);
wl_codec_status_t clear_faults_response_value_encode(const clear_faults_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t clear_faults_response_value_decode(const uint8_t *input, size_t length, clear_faults_response_value_t *out);

#define ACQUIRE_CONTROL_LEASE_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_requested_timeout_ms;
  uint32_t requested_timeout_ms;
  bool has_current_token;
  uint64_t current_token;
} acquire_control_lease_request_value_t;
#define ACQUIRE_CONTROL_LEASE_REQUEST_VALUE_SIZE (sizeof(acquire_control_lease_request_value_t))
void acquire_control_lease_request_value_clear(acquire_control_lease_request_value_t *value);
size_t acquire_control_lease_request_value_encoded_size(const acquire_control_lease_request_value_t *value);
wl_codec_status_t acquire_control_lease_request_value_encode(const acquire_control_lease_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t acquire_control_lease_request_value_decode(const uint8_t *input, size_t length, acquire_control_lease_request_value_t *out);

#define ACQUIRE_CONTROL_LEASE_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  control_lease_status_t status;
  bool has_lease_token;
  uint64_t lease_token;
  bool has_granted_timeout_ms;
  uint32_t granted_timeout_ms;
} acquire_control_lease_response_value_t;
#define ACQUIRE_CONTROL_LEASE_RESPONSE_VALUE_SIZE (sizeof(acquire_control_lease_response_value_t))
void acquire_control_lease_response_value_clear(acquire_control_lease_response_value_t *value);
size_t acquire_control_lease_response_value_encoded_size(const acquire_control_lease_response_value_t *value);
wl_codec_status_t acquire_control_lease_response_value_encode(const acquire_control_lease_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t acquire_control_lease_response_value_decode(const uint8_t *input, size_t length, acquire_control_lease_response_value_t *out);

#define RELEASE_CONTROL_LEASE_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_lease_token;
  uint64_t lease_token;
} release_control_lease_request_value_t;
#define RELEASE_CONTROL_LEASE_REQUEST_VALUE_SIZE (sizeof(release_control_lease_request_value_t))
void release_control_lease_request_value_clear(release_control_lease_request_value_t *value);
size_t release_control_lease_request_value_encoded_size(const release_control_lease_request_value_t *value);
wl_codec_status_t release_control_lease_request_value_encode(const release_control_lease_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t release_control_lease_request_value_decode(const uint8_t *input, size_t length, release_control_lease_request_value_t *out);

#define RELEASE_CONTROL_LEASE_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  control_lease_status_t status;
} release_control_lease_response_value_t;
#define RELEASE_CONTROL_LEASE_RESPONSE_VALUE_SIZE (sizeof(release_control_lease_response_value_t))
void release_control_lease_response_value_clear(release_control_lease_response_value_t *value);
size_t release_control_lease_response_value_encoded_size(const release_control_lease_response_value_t *value);
wl_codec_status_t release_control_lease_response_value_encode(const release_control_lease_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t release_control_lease_response_value_decode(const uint8_t *input, size_t length, release_control_lease_response_value_t *out);

#define GET_MOTOR_FEEDBACK_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
} get_motor_feedback_request_value_t;
#define GET_MOTOR_FEEDBACK_REQUEST_VALUE_SIZE (sizeof(get_motor_feedback_request_value_t))
void get_motor_feedback_request_value_clear(get_motor_feedback_request_value_t *value);
size_t get_motor_feedback_request_value_encoded_size(const get_motor_feedback_request_value_t *value);
wl_codec_status_t get_motor_feedback_request_value_encode(const get_motor_feedback_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t get_motor_feedback_request_value_decode(const uint8_t *input, size_t length, get_motor_feedback_request_value_t *out);

#define GET_MOTOR_FEEDBACK_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
  bool has_feedback;
  motor_feedback_value_t feedback;
} get_motor_feedback_response_value_t;
#define GET_MOTOR_FEEDBACK_RESPONSE_VALUE_SIZE (sizeof(get_motor_feedback_response_value_t))
void get_motor_feedback_response_value_clear(get_motor_feedback_response_value_t *value);
size_t get_motor_feedback_response_value_encoded_size(const get_motor_feedback_response_value_t *value);
wl_codec_status_t get_motor_feedback_response_value_encode(const get_motor_feedback_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t get_motor_feedback_response_value_decode(const uint8_t *input, size_t length, get_motor_feedback_response_value_t *out);

#define GET_DEVICE_INFO_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
} get_device_info_request_value_t;
#define GET_DEVICE_INFO_REQUEST_VALUE_SIZE (sizeof(get_device_info_request_value_t))
void get_device_info_request_value_clear(get_device_info_request_value_t *value);
size_t get_device_info_request_value_encoded_size(const get_device_info_request_value_t *value);
wl_codec_status_t get_device_info_request_value_encode(const get_device_info_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t get_device_info_request_value_decode(const uint8_t *input, size_t length, get_device_info_request_value_t *out);

#define GET_DEVICE_INFO_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_info_status_t status;
  bool has_info;
  device_info_value_t info;
} get_device_info_response_value_t;
#define GET_DEVICE_INFO_RESPONSE_VALUE_SIZE (sizeof(get_device_info_response_value_t))
void get_device_info_response_value_clear(get_device_info_response_value_t *value);
size_t get_device_info_response_value_encoded_size(const get_device_info_response_value_t *value);
wl_codec_status_t get_device_info_response_value_encode(const get_device_info_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t get_device_info_response_value_decode(const uint8_t *input, size_t length, get_device_info_response_value_t *out);

#define SET_DEVICE_INFO_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_custom_name;
  struct { size_t length; char data[32]; } custom_name;
} set_device_info_request_value_t;
#define SET_DEVICE_INFO_REQUEST_VALUE_SIZE (sizeof(set_device_info_request_value_t))
void set_device_info_request_value_clear(set_device_info_request_value_t *value);
size_t set_device_info_request_value_encoded_size(const set_device_info_request_value_t *value);
wl_codec_status_t set_device_info_request_value_encode(const set_device_info_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_device_info_request_value_decode(const uint8_t *input, size_t length, set_device_info_request_value_t *out);

#define SET_DEVICE_INFO_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_info_status_t status;
} set_device_info_response_value_t;
#define SET_DEVICE_INFO_RESPONSE_VALUE_SIZE (sizeof(set_device_info_response_value_t))
void set_device_info_response_value_clear(set_device_info_response_value_t *value);
size_t set_device_info_response_value_encoded_size(const set_device_info_response_value_t *value);
wl_codec_status_t set_device_info_response_value_encode(const set_device_info_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_device_info_response_value_decode(const uint8_t *input, size_t length, set_device_info_response_value_t *out);

#define SET_ARM_CONTROL_MODE_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_mode;
  motor_control_mode_t mode;
} set_arm_control_mode_request_value_t;
#define SET_ARM_CONTROL_MODE_REQUEST_VALUE_SIZE (sizeof(set_arm_control_mode_request_value_t))
void set_arm_control_mode_request_value_clear(set_arm_control_mode_request_value_t *value);
size_t set_arm_control_mode_request_value_encoded_size(const set_arm_control_mode_request_value_t *value);
wl_codec_status_t set_arm_control_mode_request_value_encode(const set_arm_control_mode_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_arm_control_mode_request_value_decode(const uint8_t *input, size_t length, set_arm_control_mode_request_value_t *out);

#define SET_ARM_CONTROL_MODE_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  mode_status_t status;
} set_arm_control_mode_response_value_t;
#define SET_ARM_CONTROL_MODE_RESPONSE_VALUE_SIZE (sizeof(set_arm_control_mode_response_value_t))
void set_arm_control_mode_response_value_clear(set_arm_control_mode_response_value_t *value);
size_t set_arm_control_mode_response_value_encoded_size(const set_arm_control_mode_response_value_t *value);
wl_codec_status_t set_arm_control_mode_response_value_encode(const set_arm_control_mode_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_arm_control_mode_response_value_decode(const uint8_t *input, size_t length, set_arm_control_mode_response_value_t *out);

#define SET_GRIPPER_CONTROL_MODE_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_mode;
  motor_control_mode_t mode;
} set_gripper_control_mode_request_value_t;
#define SET_GRIPPER_CONTROL_MODE_REQUEST_VALUE_SIZE (sizeof(set_gripper_control_mode_request_value_t))
void set_gripper_control_mode_request_value_clear(set_gripper_control_mode_request_value_t *value);
size_t set_gripper_control_mode_request_value_encoded_size(const set_gripper_control_mode_request_value_t *value);
wl_codec_status_t set_gripper_control_mode_request_value_encode(const set_gripper_control_mode_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_gripper_control_mode_request_value_decode(const uint8_t *input, size_t length, set_gripper_control_mode_request_value_t *out);

#define SET_GRIPPER_CONTROL_MODE_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  mode_status_t status;
} set_gripper_control_mode_response_value_t;
#define SET_GRIPPER_CONTROL_MODE_RESPONSE_VALUE_SIZE (sizeof(set_gripper_control_mode_response_value_t))
void set_gripper_control_mode_response_value_clear(set_gripper_control_mode_response_value_t *value);
size_t set_gripper_control_mode_response_value_encoded_size(const set_gripper_control_mode_response_value_t *value);
wl_codec_status_t set_gripper_control_mode_response_value_encode(const set_gripper_control_mode_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_gripper_control_mode_response_value_decode(const uint8_t *input, size_t length, set_gripper_control_mode_response_value_t *out);

#define MOTOR_REGISTER_READ_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
  bool has_register_id;
  uint8_t register_id;
} motor_register_read_request_value_t;
#define MOTOR_REGISTER_READ_REQUEST_VALUE_SIZE (sizeof(motor_register_read_request_value_t))
void motor_register_read_request_value_clear(motor_register_read_request_value_t *value);
size_t motor_register_read_request_value_encoded_size(const motor_register_read_request_value_t *value);
wl_codec_status_t motor_register_read_request_value_encode(const motor_register_read_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_register_read_request_value_decode(const uint8_t *input, size_t length, motor_register_read_request_value_t *out);

#define MOTOR_REGISTER_READ_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
  bool has_joint_id;
  uint8_t joint_id;
  bool has_register_id;
  uint8_t register_id;
  bool has_value;
  float value;
} motor_register_read_response_value_t;
#define MOTOR_REGISTER_READ_RESPONSE_VALUE_SIZE (sizeof(motor_register_read_response_value_t))
void motor_register_read_response_value_clear(motor_register_read_response_value_t *value);
size_t motor_register_read_response_value_encoded_size(const motor_register_read_response_value_t *value);
wl_codec_status_t motor_register_read_response_value_encode(const motor_register_read_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_register_read_response_value_decode(const uint8_t *input, size_t length, motor_register_read_response_value_t *out);

#define MOTOR_REGISTER_WRITE_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
  bool has_register_id;
  uint8_t register_id;
  bool has_value;
  float value;
} motor_register_write_request_value_t;
#define MOTOR_REGISTER_WRITE_REQUEST_VALUE_SIZE (sizeof(motor_register_write_request_value_t))
void motor_register_write_request_value_clear(motor_register_write_request_value_t *value);
size_t motor_register_write_request_value_encoded_size(const motor_register_write_request_value_t *value);
wl_codec_status_t motor_register_write_request_value_encode(const motor_register_write_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_register_write_request_value_decode(const uint8_t *input, size_t length, motor_register_write_request_value_t *out);

#define MOTOR_REGISTER_WRITE_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
} motor_register_write_response_value_t;
#define MOTOR_REGISTER_WRITE_RESPONSE_VALUE_SIZE (sizeof(motor_register_write_response_value_t))
void motor_register_write_response_value_clear(motor_register_write_response_value_t *value);
size_t motor_register_write_response_value_encoded_size(const motor_register_write_response_value_t *value);
wl_codec_status_t motor_register_write_response_value_encode(const motor_register_write_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_register_write_response_value_decode(const uint8_t *input, size_t length, motor_register_write_response_value_t *out);

#define MOTOR_STORE_PARAMETERS_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
} motor_store_parameters_request_value_t;
#define MOTOR_STORE_PARAMETERS_REQUEST_VALUE_SIZE (sizeof(motor_store_parameters_request_value_t))
void motor_store_parameters_request_value_clear(motor_store_parameters_request_value_t *value);
size_t motor_store_parameters_request_value_encoded_size(const motor_store_parameters_request_value_t *value);
wl_codec_status_t motor_store_parameters_request_value_encode(const motor_store_parameters_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_store_parameters_request_value_decode(const uint8_t *input, size_t length, motor_store_parameters_request_value_t *out);

#define MOTOR_STORE_PARAMETERS_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
} motor_store_parameters_response_value_t;
#define MOTOR_STORE_PARAMETERS_RESPONSE_VALUE_SIZE (sizeof(motor_store_parameters_response_value_t))
void motor_store_parameters_response_value_clear(motor_store_parameters_response_value_t *value);
size_t motor_store_parameters_response_value_encoded_size(const motor_store_parameters_response_value_t *value);
wl_codec_status_t motor_store_parameters_response_value_encode(const motor_store_parameters_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_store_parameters_response_value_decode(const uint8_t *input, size_t length, motor_store_parameters_response_value_t *out);

#define MOTOR_SET_ZERO_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
} motor_set_zero_request_value_t;
#define MOTOR_SET_ZERO_REQUEST_VALUE_SIZE (sizeof(motor_set_zero_request_value_t))
void motor_set_zero_request_value_clear(motor_set_zero_request_value_t *value);
size_t motor_set_zero_request_value_encoded_size(const motor_set_zero_request_value_t *value);
wl_codec_status_t motor_set_zero_request_value_encode(const motor_set_zero_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_set_zero_request_value_decode(const uint8_t *input, size_t length, motor_set_zero_request_value_t *out);

#define MOTOR_SET_ZERO_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
} motor_set_zero_response_value_t;
#define MOTOR_SET_ZERO_RESPONSE_VALUE_SIZE (sizeof(motor_set_zero_response_value_t))
void motor_set_zero_response_value_clear(motor_set_zero_response_value_t *value);
size_t motor_set_zero_response_value_encoded_size(const motor_set_zero_response_value_t *value);
wl_codec_status_t motor_set_zero_response_value_encode(const motor_set_zero_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t motor_set_zero_response_value_decode(const uint8_t *input, size_t length, motor_set_zero_response_value_t *out);

#define SET_ARM_MODE_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_mode;
  arm_mode_t mode;
} set_arm_mode_request_value_t;
#define SET_ARM_MODE_REQUEST_VALUE_SIZE (sizeof(set_arm_mode_request_value_t))
void set_arm_mode_request_value_clear(set_arm_mode_request_value_t *value);
size_t set_arm_mode_request_value_encoded_size(const set_arm_mode_request_value_t *value);
wl_codec_status_t set_arm_mode_request_value_encode(const set_arm_mode_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_arm_mode_request_value_decode(const uint8_t *input, size_t length, set_arm_mode_request_value_t *out);

#define SET_ARM_MODE_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  mode_status_t status;
} set_arm_mode_response_value_t;
#define SET_ARM_MODE_RESPONSE_VALUE_SIZE (sizeof(set_arm_mode_response_value_t))
void set_arm_mode_response_value_clear(set_arm_mode_response_value_t *value);
size_t set_arm_mode_response_value_encoded_size(const set_arm_mode_response_value_t *value);
wl_codec_status_t set_arm_mode_response_value_encode(const set_arm_mode_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_arm_mode_response_value_decode(const uint8_t *input, size_t length, set_arm_mode_response_value_t *out);

#define GET_DEVICE_SETTINGS_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
} get_device_settings_request_value_t;
#define GET_DEVICE_SETTINGS_REQUEST_VALUE_SIZE (sizeof(get_device_settings_request_value_t))
void get_device_settings_request_value_clear(get_device_settings_request_value_t *value);
size_t get_device_settings_request_value_encoded_size(const get_device_settings_request_value_t *value);
wl_codec_status_t get_device_settings_request_value_encode(const get_device_settings_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t get_device_settings_request_value_decode(const uint8_t *input, size_t length, get_device_settings_request_value_t *out);

#define GET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_settings_status_t status;
  bool has_settings;
  device_settings_value_t settings;
} get_device_settings_response_value_t;
#define GET_DEVICE_SETTINGS_RESPONSE_VALUE_SIZE (sizeof(get_device_settings_response_value_t))
void get_device_settings_response_value_clear(get_device_settings_response_value_t *value);
size_t get_device_settings_response_value_encoded_size(const get_device_settings_response_value_t *value);
wl_codec_status_t get_device_settings_response_value_encode(const get_device_settings_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t get_device_settings_response_value_decode(const uint8_t *input, size_t length, get_device_settings_response_value_t *out);

#define SET_DEVICE_SETTINGS_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_settings;
  device_settings_value_t settings;
} set_device_settings_request_value_t;
#define SET_DEVICE_SETTINGS_REQUEST_VALUE_SIZE (sizeof(set_device_settings_request_value_t))
void set_device_settings_request_value_clear(set_device_settings_request_value_t *value);
size_t set_device_settings_request_value_encoded_size(const set_device_settings_request_value_t *value);
wl_codec_status_t set_device_settings_request_value_encode(const set_device_settings_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_device_settings_request_value_decode(const uint8_t *input, size_t length, set_device_settings_request_value_t *out);

#define SET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_settings_status_t status;
  bool has_settings;
  device_settings_value_t settings;
} set_device_settings_response_value_t;
#define SET_DEVICE_SETTINGS_RESPONSE_VALUE_SIZE (sizeof(set_device_settings_response_value_t))
void set_device_settings_response_value_clear(set_device_settings_response_value_t *value);
size_t set_device_settings_response_value_encoded_size(const set_device_settings_response_value_t *value);
wl_codec_status_t set_device_settings_response_value_encode(const set_device_settings_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t set_device_settings_response_value_decode(const uint8_t *input, size_t length, set_device_settings_response_value_t *out);

#define JOINT_MIT_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_position;
  float position[6];
  bool has_velocity;
  float velocity[6];
  bool has_torque;
  float torque[6];
  bool has_kp;
  float kp[6];
  bool has_kd;
  float kd[6];
  bool has_dt_us;
  uint32_t dt_us;
  bool has_sequence;
  uint32_t sequence;
  bool has_gravity_compensation;
  bool gravity_compensation;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} joint_mit_command_value_t;
#define JOINT_MIT_COMMAND_VALUE_SIZE (sizeof(joint_mit_command_value_t))
void joint_mit_command_value_clear(joint_mit_command_value_t *value);
size_t joint_mit_command_value_encoded_size(const joint_mit_command_value_t *value);
wl_codec_status_t joint_mit_command_value_encode(const joint_mit_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t joint_mit_command_value_decode(const uint8_t *input, size_t length, joint_mit_command_value_t *out);

#define EMERGENCY_STOP_REQUEST_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
} emergency_stop_request_value_t;
#define EMERGENCY_STOP_REQUEST_VALUE_SIZE (sizeof(emergency_stop_request_value_t))
void emergency_stop_request_value_clear(emergency_stop_request_value_t *value);
size_t emergency_stop_request_value_encoded_size(const emergency_stop_request_value_t *value);
wl_codec_status_t emergency_stop_request_value_encode(const emergency_stop_request_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t emergency_stop_request_value_decode(const uint8_t *input, size_t length, emergency_stop_request_value_t *out);

#define EMERGENCY_STOP_RESPONSE_HAS_VALUE 1
typedef struct {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  emergency_stop_status_t status;
} emergency_stop_response_value_t;
#define EMERGENCY_STOP_RESPONSE_VALUE_SIZE (sizeof(emergency_stop_response_value_t))
void emergency_stop_response_value_clear(emergency_stop_response_value_t *value);
size_t emergency_stop_response_value_encoded_size(const emergency_stop_response_value_t *value);
wl_codec_status_t emergency_stop_response_value_encode(const emergency_stop_response_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t emergency_stop_response_value_decode(const uint8_t *input, size_t length, emergency_stop_response_value_t *out);

#define GRIPPER_MIT_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_position;
  float position;
  bool has_velocity;
  float velocity;
  bool has_torque;
  float torque;
  bool has_kp;
  float kp;
  bool has_kd;
  float kd;
  bool has_dt_us;
  uint32_t dt_us;
  bool has_sequence;
  uint32_t sequence;
  bool has_gravity_compensation;
  bool gravity_compensation;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} gripper_mit_command_value_t;
#define GRIPPER_MIT_COMMAND_VALUE_SIZE (sizeof(gripper_mit_command_value_t))
void gripper_mit_command_value_clear(gripper_mit_command_value_t *value);
size_t gripper_mit_command_value_encoded_size(const gripper_mit_command_value_t *value);
wl_codec_status_t gripper_mit_command_value_encode(const gripper_mit_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t gripper_mit_command_value_decode(const uint8_t *input, size_t length, gripper_mit_command_value_t *out);

#define JOINT_POSITION_VELOCITY_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_position;
  float position[6];
  bool has_velocity;
  float velocity[6];
  bool has_enabled_mask;
  uint8_t enabled_mask;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} joint_position_velocity_command_value_t;
#define JOINT_POSITION_VELOCITY_COMMAND_VALUE_SIZE (sizeof(joint_position_velocity_command_value_t))
void joint_position_velocity_command_value_clear(joint_position_velocity_command_value_t *value);
size_t joint_position_velocity_command_value_encoded_size(const joint_position_velocity_command_value_t *value);
wl_codec_status_t joint_position_velocity_command_value_encode(const joint_position_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t joint_position_velocity_command_value_decode(const uint8_t *input, size_t length, joint_position_velocity_command_value_t *out);

#define JOINT_VELOCITY_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_velocity;
  float velocity[6];
  bool has_enabled_mask;
  uint8_t enabled_mask;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} joint_velocity_command_value_t;
#define JOINT_VELOCITY_COMMAND_VALUE_SIZE (sizeof(joint_velocity_command_value_t))
void joint_velocity_command_value_clear(joint_velocity_command_value_t *value);
size_t joint_velocity_command_value_encoded_size(const joint_velocity_command_value_t *value);
wl_codec_status_t joint_velocity_command_value_encode(const joint_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t joint_velocity_command_value_decode(const uint8_t *input, size_t length, joint_velocity_command_value_t *out);

#define JOINT_PVT_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_position;
  float position[6];
  bool has_velocity_limit;
  float velocity_limit[6];
  bool has_current_limit_normalized;
  float current_limit_normalized[6];
  bool has_enabled_mask;
  uint8_t enabled_mask;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} joint_pvt_command_value_t;
#define JOINT_PVT_COMMAND_VALUE_SIZE (sizeof(joint_pvt_command_value_t))
void joint_pvt_command_value_clear(joint_pvt_command_value_t *value);
size_t joint_pvt_command_value_encoded_size(const joint_pvt_command_value_t *value);
wl_codec_status_t joint_pvt_command_value_encode(const joint_pvt_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t joint_pvt_command_value_decode(const uint8_t *input, size_t length, joint_pvt_command_value_t *out);

#define CARTESIAN_POSE_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_transform;
  float transform[16];
  bool has_kp;
  float kp[6];
  bool has_kd;
  float kd[6];
  bool has_dt_us;
  uint32_t dt_us;
  bool has_sequence;
  uint32_t sequence;
  bool has_gravity_compensation;
  bool gravity_compensation;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} cartesian_pose_command_value_t;
#define CARTESIAN_POSE_COMMAND_VALUE_SIZE (sizeof(cartesian_pose_command_value_t))
void cartesian_pose_command_value_clear(cartesian_pose_command_value_t *value);
size_t cartesian_pose_command_value_encoded_size(const cartesian_pose_command_value_t *value);
wl_codec_status_t cartesian_pose_command_value_encode(const cartesian_pose_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t cartesian_pose_command_value_decode(const uint8_t *input, size_t length, cartesian_pose_command_value_t *out);

#define CARTESIAN_VELOCITY_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_twist;
  float twist[6];
  bool has_kp;
  float kp[6];
  bool has_kd;
  float kd[6];
  bool has_dt_us;
  uint32_t dt_us;
  bool has_sequence;
  uint32_t sequence;
  bool has_gravity_compensation;
  bool gravity_compensation;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} cartesian_velocity_command_value_t;
#define CARTESIAN_VELOCITY_COMMAND_VALUE_SIZE (sizeof(cartesian_velocity_command_value_t))
void cartesian_velocity_command_value_clear(cartesian_velocity_command_value_t *value);
size_t cartesian_velocity_command_value_encoded_size(const cartesian_velocity_command_value_t *value);
wl_codec_status_t cartesian_velocity_command_value_encode(const cartesian_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t cartesian_velocity_command_value_decode(const uint8_t *input, size_t length, cartesian_velocity_command_value_t *out);

#define GRIPPER_POSITION_VELOCITY_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_position;
  float position;
  bool has_velocity;
  float velocity;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} gripper_position_velocity_command_value_t;
#define GRIPPER_POSITION_VELOCITY_COMMAND_VALUE_SIZE (sizeof(gripper_position_velocity_command_value_t))
void gripper_position_velocity_command_value_clear(gripper_position_velocity_command_value_t *value);
size_t gripper_position_velocity_command_value_encoded_size(const gripper_position_velocity_command_value_t *value);
wl_codec_status_t gripper_position_velocity_command_value_encode(const gripper_position_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t gripper_position_velocity_command_value_decode(const uint8_t *input, size_t length, gripper_position_velocity_command_value_t *out);

#define GRIPPER_VELOCITY_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_velocity;
  float velocity;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} gripper_velocity_command_value_t;
#define GRIPPER_VELOCITY_COMMAND_VALUE_SIZE (sizeof(gripper_velocity_command_value_t))
void gripper_velocity_command_value_clear(gripper_velocity_command_value_t *value);
size_t gripper_velocity_command_value_encoded_size(const gripper_velocity_command_value_t *value);
wl_codec_status_t gripper_velocity_command_value_encode(const gripper_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t gripper_velocity_command_value_decode(const uint8_t *input, size_t length, gripper_velocity_command_value_t *out);

#define GRIPPER_PVT_COMMAND_HAS_VALUE 1
typedef struct {
  bool has_position;
  float position;
  bool has_velocity_limit;
  float velocity_limit;
  bool has_current_limit_normalized;
  float current_limit_normalized;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
} gripper_pvt_command_value_t;
#define GRIPPER_PVT_COMMAND_VALUE_SIZE (sizeof(gripper_pvt_command_value_t))
void gripper_pvt_command_value_clear(gripper_pvt_command_value_t *value);
size_t gripper_pvt_command_value_encoded_size(const gripper_pvt_command_value_t *value);
wl_codec_status_t gripper_pvt_command_value_encode(const gripper_pvt_command_value_t *value, uint8_t *out, size_t capacity, size_t *length);
wl_codec_status_t gripper_pvt_command_value_decode(const uint8_t *input, size_t length, gripper_pvt_command_value_t *out);

#define SEMANTIC_VERSION_MESSAGE_ID 20737U
#define SEMANTIC_VERSION_HAS_MAX_ENCODED_SIZE 1
#define SEMANTIC_VERSION_MAX_ENCODED_SIZE UINT64_C(18)
#define DEVICE_INFO_MESSAGE_ID 20738U
#define DEVICE_INFO_HAS_MAX_ENCODED_SIZE 1
#define DEVICE_INFO_MAX_ENCODED_SIZE UINT64_C(154)
#define DEVICE_SETTINGS_MESSAGE_ID 20739U
#define DEVICE_SETTINGS_HAS_MAX_ENCODED_SIZE 1
#define DEVICE_SETTINGS_MAX_ENCODED_SIZE UINT64_C(203)
#define ARM_STATUS_MESSAGE_ID 24577U
#define ARM_STATUS_HAS_MAX_ENCODED_SIZE 1
#define ARM_STATUS_MAX_ENCODED_SIZE UINT64_C(233)
#define MOTOR_FEEDBACK_MESSAGE_ID 24578U
#define MOTOR_FEEDBACK_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_FEEDBACK_MAX_ENCODED_SIZE UINT64_C(128)
#define ARM_DIAGNOSTICS_MESSAGE_ID 24580U
#define ARM_DIAGNOSTICS_HAS_MAX_ENCODED_SIZE 1
#define ARM_DIAGNOSTICS_MAX_ENCODED_SIZE UINT64_C(67)
#define SET_ZERO_REQUEST_MESSAGE_ID 24839U
#define SET_ZERO_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define SET_ZERO_REQUEST_MAX_ENCODED_SIZE UINT64_C(9)
#define SET_ZERO_RESPONSE_MESSAGE_ID 24840U
#define SET_ZERO_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define SET_ZERO_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define CLEAR_ERROR_REQUEST_MESSAGE_ID 24841U
#define CLEAR_ERROR_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define CLEAR_ERROR_REQUEST_MAX_ENCODED_SIZE UINT64_C(9)
#define CLEAR_ERROR_RESPONSE_MESSAGE_ID 24842U
#define CLEAR_ERROR_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define CLEAR_ERROR_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define HOME_REQUEST_MESSAGE_ID 25089U
#define HOME_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define HOME_REQUEST_MAX_ENCODED_SIZE UINT64_C(6)
#define HOME_RESPONSE_MESSAGE_ID 25090U
#define HOME_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define HOME_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define CLEAR_FAULTS_REQUEST_MESSAGE_ID 25093U
#define CLEAR_FAULTS_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define CLEAR_FAULTS_REQUEST_MAX_ENCODED_SIZE UINT64_C(6)
#define CLEAR_FAULTS_RESPONSE_MESSAGE_ID 25094U
#define CLEAR_FAULTS_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define CLEAR_FAULTS_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID 25097U
#define ACQUIRE_CONTROL_LEASE_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define ACQUIRE_CONTROL_LEASE_REQUEST_MAX_ENCODED_SIZE UINT64_C(20)
#define ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID 25098U
#define ACQUIRE_CONTROL_LEASE_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define ACQUIRE_CONTROL_LEASE_RESPONSE_MAX_ENCODED_SIZE UINT64_C(26)
#define RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID 25099U
#define RELEASE_CONTROL_LEASE_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define RELEASE_CONTROL_LEASE_REQUEST_MAX_ENCODED_SIZE UINT64_C(15)
#define RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID 25100U
#define RELEASE_CONTROL_LEASE_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define RELEASE_CONTROL_LEASE_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID 25107U
#define GET_MOTOR_FEEDBACK_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define GET_MOTOR_FEEDBACK_REQUEST_MAX_ENCODED_SIZE UINT64_C(6)
#define GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID 25108U
#define GET_MOTOR_FEEDBACK_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define GET_MOTOR_FEEDBACK_RESPONSE_MAX_ENCODED_SIZE UINT64_C(143)
#define GET_DEVICE_INFO_REQUEST_MESSAGE_ID 25109U
#define GET_DEVICE_INFO_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define GET_DEVICE_INFO_REQUEST_MAX_ENCODED_SIZE UINT64_C(6)
#define GET_DEVICE_INFO_RESPONSE_MESSAGE_ID 25110U
#define GET_DEVICE_INFO_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define GET_DEVICE_INFO_RESPONSE_MAX_ENCODED_SIZE UINT64_C(169)
#define SET_DEVICE_INFO_REQUEST_MESSAGE_ID 25111U
#define SET_DEVICE_INFO_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define SET_DEVICE_INFO_REQUEST_MAX_ENCODED_SIZE UINT64_C(39)
#define SET_DEVICE_INFO_RESPONSE_MESSAGE_ID 25112U
#define SET_DEVICE_INFO_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define SET_DEVICE_INFO_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID 25113U
#define SET_ARM_CONTROL_MODE_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define SET_ARM_CONTROL_MODE_REQUEST_MAX_ENCODED_SIZE UINT64_C(12)
#define SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID 25114U
#define SET_ARM_CONTROL_MODE_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define SET_ARM_CONTROL_MODE_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID 25115U
#define SET_GRIPPER_CONTROL_MODE_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define SET_GRIPPER_CONTROL_MODE_REQUEST_MAX_ENCODED_SIZE UINT64_C(12)
#define SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID 25116U
#define SET_GRIPPER_CONTROL_MODE_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define SET_GRIPPER_CONTROL_MODE_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID 25117U
#define MOTOR_REGISTER_READ_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_REGISTER_READ_REQUEST_MAX_ENCODED_SIZE UINT64_C(12)
#define MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID 25118U
#define MOTOR_REGISTER_READ_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_REGISTER_READ_RESPONSE_MAX_ENCODED_SIZE UINT64_C(23)
#define MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID 25119U
#define MOTOR_REGISTER_WRITE_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_REGISTER_WRITE_REQUEST_MAX_ENCODED_SIZE UINT64_C(17)
#define MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID 25120U
#define MOTOR_REGISTER_WRITE_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_REGISTER_WRITE_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID 25121U
#define MOTOR_STORE_PARAMETERS_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_STORE_PARAMETERS_REQUEST_MAX_ENCODED_SIZE UINT64_C(9)
#define MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID 25122U
#define MOTOR_STORE_PARAMETERS_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_STORE_PARAMETERS_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define MOTOR_SET_ZERO_REQUEST_MESSAGE_ID 25123U
#define MOTOR_SET_ZERO_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_SET_ZERO_REQUEST_MAX_ENCODED_SIZE UINT64_C(9)
#define MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID 25124U
#define MOTOR_SET_ZERO_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define MOTOR_SET_ZERO_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define SET_ARM_MODE_REQUEST_MESSAGE_ID 25125U
#define SET_ARM_MODE_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define SET_ARM_MODE_REQUEST_MAX_ENCODED_SIZE UINT64_C(12)
#define SET_ARM_MODE_RESPONSE_MESSAGE_ID 25126U
#define SET_ARM_MODE_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define SET_ARM_MODE_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID 25128U
#define GET_DEVICE_SETTINGS_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define GET_DEVICE_SETTINGS_REQUEST_MAX_ENCODED_SIZE UINT64_C(6)
#define GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID 25129U
#define GET_DEVICE_SETTINGS_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define GET_DEVICE_SETTINGS_RESPONSE_MAX_ENCODED_SIZE UINT64_C(218)
#define SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID 25130U
#define SET_DEVICE_SETTINGS_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define SET_DEVICE_SETTINGS_REQUEST_MAX_ENCODED_SIZE UINT64_C(212)
#define SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID 25131U
#define SET_DEVICE_SETTINGS_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define SET_DEVICE_SETTINGS_RESPONSE_MAX_ENCODED_SIZE UINT64_C(218)
#define JOINT_MIT_COMMAND_MESSAGE_ID 25345U
#define JOINT_MIT_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define JOINT_MIT_COMMAND_MAX_ENCODED_SIZE UINT64_C(160)
#define EMERGENCY_STOP_REQUEST_MESSAGE_ID 25347U
#define EMERGENCY_STOP_REQUEST_HAS_MAX_ENCODED_SIZE 1
#define EMERGENCY_STOP_REQUEST_MAX_ENCODED_SIZE UINT64_C(6)
#define EMERGENCY_STOP_RESPONSE_MESSAGE_ID 25348U
#define EMERGENCY_STOP_RESPONSE_HAS_MAX_ENCODED_SIZE 1
#define EMERGENCY_STOP_RESPONSE_MAX_ENCODED_SIZE UINT64_C(12)
#define GRIPPER_MIT_COMMAND_MESSAGE_ID 25349U
#define GRIPPER_MIT_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define GRIPPER_MIT_COMMAND_MAX_ENCODED_SIZE UINT64_C(55)
#define JOINT_POSITION_VELOCITY_COMMAND_MESSAGE_ID 25350U
#define JOINT_POSITION_VELOCITY_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define JOINT_POSITION_VELOCITY_COMMAND_MAX_ENCODED_SIZE UINT64_C(78)
#define JOINT_VELOCITY_COMMAND_MESSAGE_ID 25351U
#define JOINT_VELOCITY_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define JOINT_VELOCITY_COMMAND_MAX_ENCODED_SIZE UINT64_C(52)
#define JOINT_PVT_COMMAND_MESSAGE_ID 25352U
#define JOINT_PVT_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define JOINT_PVT_COMMAND_MAX_ENCODED_SIZE UINT64_C(104)
#define CARTESIAN_POSE_COMMAND_MESSAGE_ID 25353U
#define CARTESIAN_POSE_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define CARTESIAN_POSE_COMMAND_MAX_ENCODED_SIZE UINT64_C(148)
#define CARTESIAN_VELOCITY_COMMAND_MESSAGE_ID 25354U
#define CARTESIAN_VELOCITY_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define CARTESIAN_VELOCITY_COMMAND_MAX_ENCODED_SIZE UINT64_C(108)
#define GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID 25361U
#define GRIPPER_POSITION_VELOCITY_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define GRIPPER_POSITION_VELOCITY_COMMAND_MAX_ENCODED_SIZE UINT64_C(33)
#define GRIPPER_VELOCITY_COMMAND_MESSAGE_ID 25362U
#define GRIPPER_VELOCITY_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define GRIPPER_VELOCITY_COMMAND_MAX_ENCODED_SIZE UINT64_C(28)
#define GRIPPER_PVT_COMMAND_MESSAGE_ID 25363U
#define GRIPPER_PVT_COMMAND_HAS_MAX_ENCODED_SIZE 1
#define GRIPPER_PVT_COMMAND_MAX_ENCODED_SIZE UINT64_C(38)
#ifdef __cplusplus
}
#endif

#endif
