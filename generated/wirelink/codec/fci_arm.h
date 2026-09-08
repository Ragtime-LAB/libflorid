#ifndef WIRELINK_GENERATED_FCI_ARM_H
#define WIRELINK_GENERATED_FCI_ARM_H

/* Advanced borrowed codec and explicit value/view conversions. */
#include "fci_arm_values.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct semantic_version semantic_version_t;
typedef struct device_info device_info_t;
typedef struct device_settings device_settings_t;
typedef struct arm_status arm_status_t;
typedef struct motor_feedback motor_feedback_t;
typedef struct arm_diagnostics arm_diagnostics_t;
typedef struct set_zero_request set_zero_request_t;
typedef struct set_zero_response set_zero_response_t;
typedef struct clear_error_request clear_error_request_t;
typedef struct clear_error_response clear_error_response_t;
typedef struct home_request home_request_t;
typedef struct home_response home_response_t;
typedef struct clear_faults_request clear_faults_request_t;
typedef struct clear_faults_response clear_faults_response_t;
typedef struct acquire_control_lease_request acquire_control_lease_request_t;
typedef struct acquire_control_lease_response acquire_control_lease_response_t;
typedef struct release_control_lease_request release_control_lease_request_t;
typedef struct release_control_lease_response release_control_lease_response_t;
typedef struct get_motor_feedback_request get_motor_feedback_request_t;
typedef struct get_motor_feedback_response get_motor_feedback_response_t;
typedef struct get_device_info_request get_device_info_request_t;
typedef struct get_device_info_response get_device_info_response_t;
typedef struct set_device_info_request set_device_info_request_t;
typedef struct set_device_info_response set_device_info_response_t;
typedef struct set_arm_control_mode_request set_arm_control_mode_request_t;
typedef struct set_arm_control_mode_response set_arm_control_mode_response_t;
typedef struct set_gripper_control_mode_request set_gripper_control_mode_request_t;
typedef struct set_gripper_control_mode_response set_gripper_control_mode_response_t;
typedef struct motor_register_read_request motor_register_read_request_t;
typedef struct motor_register_read_response motor_register_read_response_t;
typedef struct motor_register_write_request motor_register_write_request_t;
typedef struct motor_register_write_response motor_register_write_response_t;
typedef struct motor_store_parameters_request motor_store_parameters_request_t;
typedef struct motor_store_parameters_response motor_store_parameters_response_t;
typedef struct motor_set_zero_request motor_set_zero_request_t;
typedef struct motor_set_zero_response motor_set_zero_response_t;
typedef struct set_arm_mode_request set_arm_mode_request_t;
typedef struct set_arm_mode_response set_arm_mode_response_t;
typedef struct get_device_settings_request get_device_settings_request_t;
typedef struct get_device_settings_response get_device_settings_response_t;
typedef struct set_device_settings_request set_device_settings_request_t;
typedef struct set_device_settings_response set_device_settings_response_t;
typedef struct joint_mit_command joint_mit_command_t;
typedef struct emergency_stop_request emergency_stop_request_t;
typedef struct emergency_stop_response emergency_stop_response_t;
typedef struct gripper_mit_command gripper_mit_command_t;
typedef struct joint_position_velocity_command joint_position_velocity_command_t;
typedef struct joint_velocity_command joint_velocity_command_t;
typedef struct joint_pvt_command joint_pvt_command_t;
typedef struct cartesian_pose_command cartesian_pose_command_t;
typedef struct cartesian_velocity_command cartesian_velocity_command_t;
typedef struct gripper_position_velocity_command gripper_position_velocity_command_t;
typedef struct gripper_velocity_command gripper_velocity_command_t;
typedef struct gripper_pvt_command gripper_pvt_command_t;

struct semantic_version {
  bool has_major;
  uint32_t major;
  bool has_minor;
  uint32_t minor;
  bool has_patch;
  uint32_t patch;
};

struct device_info {
  bool has_protocol_version;
  semantic_version_t protocol_version;
  bool has_firmware_version;
  semantic_version_t firmware_version;
  bool has_board_name;
  wl_codec_string_t board_name;
  bool has_custom_name;
  wl_codec_string_t custom_name;
  bool has_firmware_type;
  firmware_type_t firmware_type;
  bool has_serial;
  wl_codec_string_t serial;
  bool has_command_capabilities;
  uint64_t command_capabilities;
};

struct device_settings {
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
};

struct arm_status {
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
};

struct motor_feedback {
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
};

struct arm_diagnostics {
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
};

struct set_zero_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
};

struct set_zero_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  fault_operation_status_t status;
};

struct clear_error_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
};

struct clear_error_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  fault_operation_status_t status;
};

struct home_request {
  bool has_operation_id;
  uint32_t operation_id;
};

struct home_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  home_status_t status;
};

struct clear_faults_request {
  bool has_operation_id;
  uint32_t operation_id;
};

struct clear_faults_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  fault_operation_status_t status;
};

struct acquire_control_lease_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_requested_timeout_ms;
  uint32_t requested_timeout_ms;
  bool has_current_token;
  uint64_t current_token;
};

struct acquire_control_lease_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  control_lease_status_t status;
  bool has_lease_token;
  uint64_t lease_token;
  bool has_granted_timeout_ms;
  uint32_t granted_timeout_ms;
};

struct release_control_lease_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_lease_token;
  uint64_t lease_token;
};

struct release_control_lease_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  control_lease_status_t status;
};

struct get_motor_feedback_request {
  bool has_operation_id;
  uint32_t operation_id;
};

struct get_motor_feedback_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
  bool has_feedback;
  motor_feedback_t feedback;
};

struct get_device_info_request {
  bool has_operation_id;
  uint32_t operation_id;
};

struct get_device_info_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_info_status_t status;
  bool has_info;
  device_info_t info;
};

struct set_device_info_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_custom_name;
  wl_codec_string_t custom_name;
};

struct set_device_info_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_info_status_t status;
};

struct set_arm_control_mode_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_mode;
  motor_control_mode_t mode;
};

struct set_arm_control_mode_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  mode_status_t status;
};

struct set_gripper_control_mode_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_mode;
  motor_control_mode_t mode;
};

struct set_gripper_control_mode_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  mode_status_t status;
};

struct motor_register_read_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
  bool has_register_id;
  uint8_t register_id;
};

struct motor_register_read_response {
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
};

struct motor_register_write_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
  bool has_register_id;
  uint8_t register_id;
  bool has_value;
  float value;
};

struct motor_register_write_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
};

struct motor_store_parameters_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
};

struct motor_store_parameters_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
};

struct motor_set_zero_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_joint_id;
  uint8_t joint_id;
};

struct motor_set_zero_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  motor_operation_status_t status;
};

struct set_arm_mode_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_mode;
  arm_mode_t mode;
};

struct set_arm_mode_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  mode_status_t status;
};

struct get_device_settings_request {
  bool has_operation_id;
  uint32_t operation_id;
};

struct get_device_settings_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_settings_status_t status;
  bool has_settings;
  device_settings_t settings;
};

struct set_device_settings_request {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_settings;
  device_settings_t settings;
};

struct set_device_settings_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  device_settings_status_t status;
  bool has_settings;
  device_settings_t settings;
};

struct joint_mit_command {
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
};

struct emergency_stop_request {
  bool has_operation_id;
  uint32_t operation_id;
};

struct emergency_stop_response {
  bool has_operation_id;
  uint32_t operation_id;
  bool has_status;
  emergency_stop_status_t status;
};

struct gripper_mit_command {
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
};

struct joint_position_velocity_command {
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
};

struct joint_velocity_command {
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
};

struct joint_pvt_command {
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
};

struct cartesian_pose_command {
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
};

struct cartesian_velocity_command {
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
};

struct gripper_position_velocity_command {
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
};

struct gripper_velocity_command {
  bool has_velocity;
  float velocity;
  bool has_sequence;
  uint32_t sequence;
  bool has_sdk_timestamp_us;
  uint64_t sdk_timestamp_us;
  bool has_lease_token;
  uint64_t lease_token;
};

struct gripper_pvt_command {
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
};

void semantic_version_clear(semantic_version_t *value);
size_t semantic_version_encoded_size(const semantic_version_t *value);
wl_codec_status_t semantic_version_encode(const semantic_version_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t semantic_version_decode(const uint8_t *input, size_t input_length, semantic_version_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t semantic_version_value_from_view(const semantic_version_t *view, semantic_version_value_t *out);
wl_codec_status_t semantic_version_value_to_view(const semantic_version_value_t *value, semantic_version_t *out);

void device_info_clear(device_info_t *value);
size_t device_info_encoded_size(const device_info_t *value);
wl_codec_status_t device_info_encode(const device_info_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t device_info_decode(const uint8_t *input, size_t input_length, device_info_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t device_info_value_from_view(const device_info_t *view, device_info_value_t *out);
wl_codec_status_t device_info_value_to_view(const device_info_value_t *value, device_info_t *out);

void device_settings_clear(device_settings_t *value);
size_t device_settings_encoded_size(const device_settings_t *value);
wl_codec_status_t device_settings_encode(const device_settings_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t device_settings_decode(const uint8_t *input, size_t input_length, device_settings_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t device_settings_value_from_view(const device_settings_t *view, device_settings_value_t *out);
wl_codec_status_t device_settings_value_to_view(const device_settings_value_t *value, device_settings_t *out);

void arm_status_clear(arm_status_t *value);
size_t arm_status_encoded_size(const arm_status_t *value);
wl_codec_status_t arm_status_encode(const arm_status_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t arm_status_decode(const uint8_t *input, size_t input_length, arm_status_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t arm_status_value_from_view(const arm_status_t *view, arm_status_value_t *out);
wl_codec_status_t arm_status_value_to_view(const arm_status_value_t *value, arm_status_t *out);

void motor_feedback_clear(motor_feedback_t *value);
size_t motor_feedback_encoded_size(const motor_feedback_t *value);
wl_codec_status_t motor_feedback_encode(const motor_feedback_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_feedback_decode(const uint8_t *input, size_t input_length, motor_feedback_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_feedback_value_from_view(const motor_feedback_t *view, motor_feedback_value_t *out);
wl_codec_status_t motor_feedback_value_to_view(const motor_feedback_value_t *value, motor_feedback_t *out);

void arm_diagnostics_clear(arm_diagnostics_t *value);
size_t arm_diagnostics_encoded_size(const arm_diagnostics_t *value);
wl_codec_status_t arm_diagnostics_encode(const arm_diagnostics_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t arm_diagnostics_decode(const uint8_t *input, size_t input_length, arm_diagnostics_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t arm_diagnostics_value_from_view(const arm_diagnostics_t *view, arm_diagnostics_value_t *out);
wl_codec_status_t arm_diagnostics_value_to_view(const arm_diagnostics_value_t *value, arm_diagnostics_t *out);

void set_zero_request_clear(set_zero_request_t *value);
size_t set_zero_request_encoded_size(const set_zero_request_t *value);
wl_codec_status_t set_zero_request_encode(const set_zero_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_zero_request_decode(const uint8_t *input, size_t input_length, set_zero_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_zero_request_value_from_view(const set_zero_request_t *view, set_zero_request_value_t *out);
wl_codec_status_t set_zero_request_value_to_view(const set_zero_request_value_t *value, set_zero_request_t *out);

void set_zero_response_clear(set_zero_response_t *value);
size_t set_zero_response_encoded_size(const set_zero_response_t *value);
wl_codec_status_t set_zero_response_encode(const set_zero_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_zero_response_decode(const uint8_t *input, size_t input_length, set_zero_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_zero_response_value_from_view(const set_zero_response_t *view, set_zero_response_value_t *out);
wl_codec_status_t set_zero_response_value_to_view(const set_zero_response_value_t *value, set_zero_response_t *out);

void clear_error_request_clear(clear_error_request_t *value);
size_t clear_error_request_encoded_size(const clear_error_request_t *value);
wl_codec_status_t clear_error_request_encode(const clear_error_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t clear_error_request_decode(const uint8_t *input, size_t input_length, clear_error_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t clear_error_request_value_from_view(const clear_error_request_t *view, clear_error_request_value_t *out);
wl_codec_status_t clear_error_request_value_to_view(const clear_error_request_value_t *value, clear_error_request_t *out);

void clear_error_response_clear(clear_error_response_t *value);
size_t clear_error_response_encoded_size(const clear_error_response_t *value);
wl_codec_status_t clear_error_response_encode(const clear_error_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t clear_error_response_decode(const uint8_t *input, size_t input_length, clear_error_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t clear_error_response_value_from_view(const clear_error_response_t *view, clear_error_response_value_t *out);
wl_codec_status_t clear_error_response_value_to_view(const clear_error_response_value_t *value, clear_error_response_t *out);

void home_request_clear(home_request_t *value);
size_t home_request_encoded_size(const home_request_t *value);
wl_codec_status_t home_request_encode(const home_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t home_request_decode(const uint8_t *input, size_t input_length, home_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t home_request_value_from_view(const home_request_t *view, home_request_value_t *out);
wl_codec_status_t home_request_value_to_view(const home_request_value_t *value, home_request_t *out);

void home_response_clear(home_response_t *value);
size_t home_response_encoded_size(const home_response_t *value);
wl_codec_status_t home_response_encode(const home_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t home_response_decode(const uint8_t *input, size_t input_length, home_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t home_response_value_from_view(const home_response_t *view, home_response_value_t *out);
wl_codec_status_t home_response_value_to_view(const home_response_value_t *value, home_response_t *out);

void clear_faults_request_clear(clear_faults_request_t *value);
size_t clear_faults_request_encoded_size(const clear_faults_request_t *value);
wl_codec_status_t clear_faults_request_encode(const clear_faults_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t clear_faults_request_decode(const uint8_t *input, size_t input_length, clear_faults_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t clear_faults_request_value_from_view(const clear_faults_request_t *view, clear_faults_request_value_t *out);
wl_codec_status_t clear_faults_request_value_to_view(const clear_faults_request_value_t *value, clear_faults_request_t *out);

void clear_faults_response_clear(clear_faults_response_t *value);
size_t clear_faults_response_encoded_size(const clear_faults_response_t *value);
wl_codec_status_t clear_faults_response_encode(const clear_faults_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t clear_faults_response_decode(const uint8_t *input, size_t input_length, clear_faults_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t clear_faults_response_value_from_view(const clear_faults_response_t *view, clear_faults_response_value_t *out);
wl_codec_status_t clear_faults_response_value_to_view(const clear_faults_response_value_t *value, clear_faults_response_t *out);

void acquire_control_lease_request_clear(acquire_control_lease_request_t *value);
size_t acquire_control_lease_request_encoded_size(const acquire_control_lease_request_t *value);
wl_codec_status_t acquire_control_lease_request_encode(const acquire_control_lease_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t acquire_control_lease_request_decode(const uint8_t *input, size_t input_length, acquire_control_lease_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t acquire_control_lease_request_value_from_view(const acquire_control_lease_request_t *view, acquire_control_lease_request_value_t *out);
wl_codec_status_t acquire_control_lease_request_value_to_view(const acquire_control_lease_request_value_t *value, acquire_control_lease_request_t *out);

void acquire_control_lease_response_clear(acquire_control_lease_response_t *value);
size_t acquire_control_lease_response_encoded_size(const acquire_control_lease_response_t *value);
wl_codec_status_t acquire_control_lease_response_encode(const acquire_control_lease_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t acquire_control_lease_response_decode(const uint8_t *input, size_t input_length, acquire_control_lease_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t acquire_control_lease_response_value_from_view(const acquire_control_lease_response_t *view, acquire_control_lease_response_value_t *out);
wl_codec_status_t acquire_control_lease_response_value_to_view(const acquire_control_lease_response_value_t *value, acquire_control_lease_response_t *out);

void release_control_lease_request_clear(release_control_lease_request_t *value);
size_t release_control_lease_request_encoded_size(const release_control_lease_request_t *value);
wl_codec_status_t release_control_lease_request_encode(const release_control_lease_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t release_control_lease_request_decode(const uint8_t *input, size_t input_length, release_control_lease_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t release_control_lease_request_value_from_view(const release_control_lease_request_t *view, release_control_lease_request_value_t *out);
wl_codec_status_t release_control_lease_request_value_to_view(const release_control_lease_request_value_t *value, release_control_lease_request_t *out);

void release_control_lease_response_clear(release_control_lease_response_t *value);
size_t release_control_lease_response_encoded_size(const release_control_lease_response_t *value);
wl_codec_status_t release_control_lease_response_encode(const release_control_lease_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t release_control_lease_response_decode(const uint8_t *input, size_t input_length, release_control_lease_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t release_control_lease_response_value_from_view(const release_control_lease_response_t *view, release_control_lease_response_value_t *out);
wl_codec_status_t release_control_lease_response_value_to_view(const release_control_lease_response_value_t *value, release_control_lease_response_t *out);

void get_motor_feedback_request_clear(get_motor_feedback_request_t *value);
size_t get_motor_feedback_request_encoded_size(const get_motor_feedback_request_t *value);
wl_codec_status_t get_motor_feedback_request_encode(const get_motor_feedback_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t get_motor_feedback_request_decode(const uint8_t *input, size_t input_length, get_motor_feedback_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t get_motor_feedback_request_value_from_view(const get_motor_feedback_request_t *view, get_motor_feedback_request_value_t *out);
wl_codec_status_t get_motor_feedback_request_value_to_view(const get_motor_feedback_request_value_t *value, get_motor_feedback_request_t *out);

void get_motor_feedback_response_clear(get_motor_feedback_response_t *value);
size_t get_motor_feedback_response_encoded_size(const get_motor_feedback_response_t *value);
wl_codec_status_t get_motor_feedback_response_encode(const get_motor_feedback_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t get_motor_feedback_response_decode(const uint8_t *input, size_t input_length, get_motor_feedback_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t get_motor_feedback_response_value_from_view(const get_motor_feedback_response_t *view, get_motor_feedback_response_value_t *out);
wl_codec_status_t get_motor_feedback_response_value_to_view(const get_motor_feedback_response_value_t *value, get_motor_feedback_response_t *out);

void get_device_info_request_clear(get_device_info_request_t *value);
size_t get_device_info_request_encoded_size(const get_device_info_request_t *value);
wl_codec_status_t get_device_info_request_encode(const get_device_info_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t get_device_info_request_decode(const uint8_t *input, size_t input_length, get_device_info_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t get_device_info_request_value_from_view(const get_device_info_request_t *view, get_device_info_request_value_t *out);
wl_codec_status_t get_device_info_request_value_to_view(const get_device_info_request_value_t *value, get_device_info_request_t *out);

void get_device_info_response_clear(get_device_info_response_t *value);
size_t get_device_info_response_encoded_size(const get_device_info_response_t *value);
wl_codec_status_t get_device_info_response_encode(const get_device_info_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t get_device_info_response_decode(const uint8_t *input, size_t input_length, get_device_info_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t get_device_info_response_value_from_view(const get_device_info_response_t *view, get_device_info_response_value_t *out);
wl_codec_status_t get_device_info_response_value_to_view(const get_device_info_response_value_t *value, get_device_info_response_t *out);

void set_device_info_request_clear(set_device_info_request_t *value);
size_t set_device_info_request_encoded_size(const set_device_info_request_t *value);
wl_codec_status_t set_device_info_request_encode(const set_device_info_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_device_info_request_decode(const uint8_t *input, size_t input_length, set_device_info_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_device_info_request_value_from_view(const set_device_info_request_t *view, set_device_info_request_value_t *out);
wl_codec_status_t set_device_info_request_value_to_view(const set_device_info_request_value_t *value, set_device_info_request_t *out);

void set_device_info_response_clear(set_device_info_response_t *value);
size_t set_device_info_response_encoded_size(const set_device_info_response_t *value);
wl_codec_status_t set_device_info_response_encode(const set_device_info_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_device_info_response_decode(const uint8_t *input, size_t input_length, set_device_info_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_device_info_response_value_from_view(const set_device_info_response_t *view, set_device_info_response_value_t *out);
wl_codec_status_t set_device_info_response_value_to_view(const set_device_info_response_value_t *value, set_device_info_response_t *out);

void set_arm_control_mode_request_clear(set_arm_control_mode_request_t *value);
size_t set_arm_control_mode_request_encoded_size(const set_arm_control_mode_request_t *value);
wl_codec_status_t set_arm_control_mode_request_encode(const set_arm_control_mode_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_arm_control_mode_request_decode(const uint8_t *input, size_t input_length, set_arm_control_mode_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_arm_control_mode_request_value_from_view(const set_arm_control_mode_request_t *view, set_arm_control_mode_request_value_t *out);
wl_codec_status_t set_arm_control_mode_request_value_to_view(const set_arm_control_mode_request_value_t *value, set_arm_control_mode_request_t *out);

void set_arm_control_mode_response_clear(set_arm_control_mode_response_t *value);
size_t set_arm_control_mode_response_encoded_size(const set_arm_control_mode_response_t *value);
wl_codec_status_t set_arm_control_mode_response_encode(const set_arm_control_mode_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_arm_control_mode_response_decode(const uint8_t *input, size_t input_length, set_arm_control_mode_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_arm_control_mode_response_value_from_view(const set_arm_control_mode_response_t *view, set_arm_control_mode_response_value_t *out);
wl_codec_status_t set_arm_control_mode_response_value_to_view(const set_arm_control_mode_response_value_t *value, set_arm_control_mode_response_t *out);

void set_gripper_control_mode_request_clear(set_gripper_control_mode_request_t *value);
size_t set_gripper_control_mode_request_encoded_size(const set_gripper_control_mode_request_t *value);
wl_codec_status_t set_gripper_control_mode_request_encode(const set_gripper_control_mode_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_gripper_control_mode_request_decode(const uint8_t *input, size_t input_length, set_gripper_control_mode_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_gripper_control_mode_request_value_from_view(const set_gripper_control_mode_request_t *view, set_gripper_control_mode_request_value_t *out);
wl_codec_status_t set_gripper_control_mode_request_value_to_view(const set_gripper_control_mode_request_value_t *value, set_gripper_control_mode_request_t *out);

void set_gripper_control_mode_response_clear(set_gripper_control_mode_response_t *value);
size_t set_gripper_control_mode_response_encoded_size(const set_gripper_control_mode_response_t *value);
wl_codec_status_t set_gripper_control_mode_response_encode(const set_gripper_control_mode_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_gripper_control_mode_response_decode(const uint8_t *input, size_t input_length, set_gripper_control_mode_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_gripper_control_mode_response_value_from_view(const set_gripper_control_mode_response_t *view, set_gripper_control_mode_response_value_t *out);
wl_codec_status_t set_gripper_control_mode_response_value_to_view(const set_gripper_control_mode_response_value_t *value, set_gripper_control_mode_response_t *out);

void motor_register_read_request_clear(motor_register_read_request_t *value);
size_t motor_register_read_request_encoded_size(const motor_register_read_request_t *value);
wl_codec_status_t motor_register_read_request_encode(const motor_register_read_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_register_read_request_decode(const uint8_t *input, size_t input_length, motor_register_read_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_register_read_request_value_from_view(const motor_register_read_request_t *view, motor_register_read_request_value_t *out);
wl_codec_status_t motor_register_read_request_value_to_view(const motor_register_read_request_value_t *value, motor_register_read_request_t *out);

void motor_register_read_response_clear(motor_register_read_response_t *value);
size_t motor_register_read_response_encoded_size(const motor_register_read_response_t *value);
wl_codec_status_t motor_register_read_response_encode(const motor_register_read_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_register_read_response_decode(const uint8_t *input, size_t input_length, motor_register_read_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_register_read_response_value_from_view(const motor_register_read_response_t *view, motor_register_read_response_value_t *out);
wl_codec_status_t motor_register_read_response_value_to_view(const motor_register_read_response_value_t *value, motor_register_read_response_t *out);

void motor_register_write_request_clear(motor_register_write_request_t *value);
size_t motor_register_write_request_encoded_size(const motor_register_write_request_t *value);
wl_codec_status_t motor_register_write_request_encode(const motor_register_write_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_register_write_request_decode(const uint8_t *input, size_t input_length, motor_register_write_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_register_write_request_value_from_view(const motor_register_write_request_t *view, motor_register_write_request_value_t *out);
wl_codec_status_t motor_register_write_request_value_to_view(const motor_register_write_request_value_t *value, motor_register_write_request_t *out);

void motor_register_write_response_clear(motor_register_write_response_t *value);
size_t motor_register_write_response_encoded_size(const motor_register_write_response_t *value);
wl_codec_status_t motor_register_write_response_encode(const motor_register_write_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_register_write_response_decode(const uint8_t *input, size_t input_length, motor_register_write_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_register_write_response_value_from_view(const motor_register_write_response_t *view, motor_register_write_response_value_t *out);
wl_codec_status_t motor_register_write_response_value_to_view(const motor_register_write_response_value_t *value, motor_register_write_response_t *out);

void motor_store_parameters_request_clear(motor_store_parameters_request_t *value);
size_t motor_store_parameters_request_encoded_size(const motor_store_parameters_request_t *value);
wl_codec_status_t motor_store_parameters_request_encode(const motor_store_parameters_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_store_parameters_request_decode(const uint8_t *input, size_t input_length, motor_store_parameters_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_store_parameters_request_value_from_view(const motor_store_parameters_request_t *view, motor_store_parameters_request_value_t *out);
wl_codec_status_t motor_store_parameters_request_value_to_view(const motor_store_parameters_request_value_t *value, motor_store_parameters_request_t *out);

void motor_store_parameters_response_clear(motor_store_parameters_response_t *value);
size_t motor_store_parameters_response_encoded_size(const motor_store_parameters_response_t *value);
wl_codec_status_t motor_store_parameters_response_encode(const motor_store_parameters_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_store_parameters_response_decode(const uint8_t *input, size_t input_length, motor_store_parameters_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_store_parameters_response_value_from_view(const motor_store_parameters_response_t *view, motor_store_parameters_response_value_t *out);
wl_codec_status_t motor_store_parameters_response_value_to_view(const motor_store_parameters_response_value_t *value, motor_store_parameters_response_t *out);

void motor_set_zero_request_clear(motor_set_zero_request_t *value);
size_t motor_set_zero_request_encoded_size(const motor_set_zero_request_t *value);
wl_codec_status_t motor_set_zero_request_encode(const motor_set_zero_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_set_zero_request_decode(const uint8_t *input, size_t input_length, motor_set_zero_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_set_zero_request_value_from_view(const motor_set_zero_request_t *view, motor_set_zero_request_value_t *out);
wl_codec_status_t motor_set_zero_request_value_to_view(const motor_set_zero_request_value_t *value, motor_set_zero_request_t *out);

void motor_set_zero_response_clear(motor_set_zero_response_t *value);
size_t motor_set_zero_response_encoded_size(const motor_set_zero_response_t *value);
wl_codec_status_t motor_set_zero_response_encode(const motor_set_zero_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t motor_set_zero_response_decode(const uint8_t *input, size_t input_length, motor_set_zero_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t motor_set_zero_response_value_from_view(const motor_set_zero_response_t *view, motor_set_zero_response_value_t *out);
wl_codec_status_t motor_set_zero_response_value_to_view(const motor_set_zero_response_value_t *value, motor_set_zero_response_t *out);

void set_arm_mode_request_clear(set_arm_mode_request_t *value);
size_t set_arm_mode_request_encoded_size(const set_arm_mode_request_t *value);
wl_codec_status_t set_arm_mode_request_encode(const set_arm_mode_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_arm_mode_request_decode(const uint8_t *input, size_t input_length, set_arm_mode_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_arm_mode_request_value_from_view(const set_arm_mode_request_t *view, set_arm_mode_request_value_t *out);
wl_codec_status_t set_arm_mode_request_value_to_view(const set_arm_mode_request_value_t *value, set_arm_mode_request_t *out);

void set_arm_mode_response_clear(set_arm_mode_response_t *value);
size_t set_arm_mode_response_encoded_size(const set_arm_mode_response_t *value);
wl_codec_status_t set_arm_mode_response_encode(const set_arm_mode_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_arm_mode_response_decode(const uint8_t *input, size_t input_length, set_arm_mode_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_arm_mode_response_value_from_view(const set_arm_mode_response_t *view, set_arm_mode_response_value_t *out);
wl_codec_status_t set_arm_mode_response_value_to_view(const set_arm_mode_response_value_t *value, set_arm_mode_response_t *out);

void get_device_settings_request_clear(get_device_settings_request_t *value);
size_t get_device_settings_request_encoded_size(const get_device_settings_request_t *value);
wl_codec_status_t get_device_settings_request_encode(const get_device_settings_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t get_device_settings_request_decode(const uint8_t *input, size_t input_length, get_device_settings_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t get_device_settings_request_value_from_view(const get_device_settings_request_t *view, get_device_settings_request_value_t *out);
wl_codec_status_t get_device_settings_request_value_to_view(const get_device_settings_request_value_t *value, get_device_settings_request_t *out);

void get_device_settings_response_clear(get_device_settings_response_t *value);
size_t get_device_settings_response_encoded_size(const get_device_settings_response_t *value);
wl_codec_status_t get_device_settings_response_encode(const get_device_settings_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t get_device_settings_response_decode(const uint8_t *input, size_t input_length, get_device_settings_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t get_device_settings_response_value_from_view(const get_device_settings_response_t *view, get_device_settings_response_value_t *out);
wl_codec_status_t get_device_settings_response_value_to_view(const get_device_settings_response_value_t *value, get_device_settings_response_t *out);

void set_device_settings_request_clear(set_device_settings_request_t *value);
size_t set_device_settings_request_encoded_size(const set_device_settings_request_t *value);
wl_codec_status_t set_device_settings_request_encode(const set_device_settings_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_device_settings_request_decode(const uint8_t *input, size_t input_length, set_device_settings_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_device_settings_request_value_from_view(const set_device_settings_request_t *view, set_device_settings_request_value_t *out);
wl_codec_status_t set_device_settings_request_value_to_view(const set_device_settings_request_value_t *value, set_device_settings_request_t *out);

void set_device_settings_response_clear(set_device_settings_response_t *value);
size_t set_device_settings_response_encoded_size(const set_device_settings_response_t *value);
wl_codec_status_t set_device_settings_response_encode(const set_device_settings_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t set_device_settings_response_decode(const uint8_t *input, size_t input_length, set_device_settings_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t set_device_settings_response_value_from_view(const set_device_settings_response_t *view, set_device_settings_response_value_t *out);
wl_codec_status_t set_device_settings_response_value_to_view(const set_device_settings_response_value_t *value, set_device_settings_response_t *out);

void joint_mit_command_clear(joint_mit_command_t *value);
size_t joint_mit_command_encoded_size(const joint_mit_command_t *value);
wl_codec_status_t joint_mit_command_encode(const joint_mit_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t joint_mit_command_decode(const uint8_t *input, size_t input_length, joint_mit_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t joint_mit_command_value_from_view(const joint_mit_command_t *view, joint_mit_command_value_t *out);
wl_codec_status_t joint_mit_command_value_to_view(const joint_mit_command_value_t *value, joint_mit_command_t *out);

void emergency_stop_request_clear(emergency_stop_request_t *value);
size_t emergency_stop_request_encoded_size(const emergency_stop_request_t *value);
wl_codec_status_t emergency_stop_request_encode(const emergency_stop_request_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t emergency_stop_request_decode(const uint8_t *input, size_t input_length, emergency_stop_request_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t emergency_stop_request_value_from_view(const emergency_stop_request_t *view, emergency_stop_request_value_t *out);
wl_codec_status_t emergency_stop_request_value_to_view(const emergency_stop_request_value_t *value, emergency_stop_request_t *out);

void emergency_stop_response_clear(emergency_stop_response_t *value);
size_t emergency_stop_response_encoded_size(const emergency_stop_response_t *value);
wl_codec_status_t emergency_stop_response_encode(const emergency_stop_response_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t emergency_stop_response_decode(const uint8_t *input, size_t input_length, emergency_stop_response_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t emergency_stop_response_value_from_view(const emergency_stop_response_t *view, emergency_stop_response_value_t *out);
wl_codec_status_t emergency_stop_response_value_to_view(const emergency_stop_response_value_t *value, emergency_stop_response_t *out);

void gripper_mit_command_clear(gripper_mit_command_t *value);
size_t gripper_mit_command_encoded_size(const gripper_mit_command_t *value);
wl_codec_status_t gripper_mit_command_encode(const gripper_mit_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t gripper_mit_command_decode(const uint8_t *input, size_t input_length, gripper_mit_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t gripper_mit_command_value_from_view(const gripper_mit_command_t *view, gripper_mit_command_value_t *out);
wl_codec_status_t gripper_mit_command_value_to_view(const gripper_mit_command_value_t *value, gripper_mit_command_t *out);

void joint_position_velocity_command_clear(joint_position_velocity_command_t *value);
size_t joint_position_velocity_command_encoded_size(const joint_position_velocity_command_t *value);
wl_codec_status_t joint_position_velocity_command_encode(const joint_position_velocity_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t joint_position_velocity_command_decode(const uint8_t *input, size_t input_length, joint_position_velocity_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t joint_position_velocity_command_value_from_view(const joint_position_velocity_command_t *view, joint_position_velocity_command_value_t *out);
wl_codec_status_t joint_position_velocity_command_value_to_view(const joint_position_velocity_command_value_t *value, joint_position_velocity_command_t *out);

void joint_velocity_command_clear(joint_velocity_command_t *value);
size_t joint_velocity_command_encoded_size(const joint_velocity_command_t *value);
wl_codec_status_t joint_velocity_command_encode(const joint_velocity_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t joint_velocity_command_decode(const uint8_t *input, size_t input_length, joint_velocity_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t joint_velocity_command_value_from_view(const joint_velocity_command_t *view, joint_velocity_command_value_t *out);
wl_codec_status_t joint_velocity_command_value_to_view(const joint_velocity_command_value_t *value, joint_velocity_command_t *out);

void joint_pvt_command_clear(joint_pvt_command_t *value);
size_t joint_pvt_command_encoded_size(const joint_pvt_command_t *value);
wl_codec_status_t joint_pvt_command_encode(const joint_pvt_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t joint_pvt_command_decode(const uint8_t *input, size_t input_length, joint_pvt_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t joint_pvt_command_value_from_view(const joint_pvt_command_t *view, joint_pvt_command_value_t *out);
wl_codec_status_t joint_pvt_command_value_to_view(const joint_pvt_command_value_t *value, joint_pvt_command_t *out);

void cartesian_pose_command_clear(cartesian_pose_command_t *value);
size_t cartesian_pose_command_encoded_size(const cartesian_pose_command_t *value);
wl_codec_status_t cartesian_pose_command_encode(const cartesian_pose_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t cartesian_pose_command_decode(const uint8_t *input, size_t input_length, cartesian_pose_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t cartesian_pose_command_value_from_view(const cartesian_pose_command_t *view, cartesian_pose_command_value_t *out);
wl_codec_status_t cartesian_pose_command_value_to_view(const cartesian_pose_command_value_t *value, cartesian_pose_command_t *out);

void cartesian_velocity_command_clear(cartesian_velocity_command_t *value);
size_t cartesian_velocity_command_encoded_size(const cartesian_velocity_command_t *value);
wl_codec_status_t cartesian_velocity_command_encode(const cartesian_velocity_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t cartesian_velocity_command_decode(const uint8_t *input, size_t input_length, cartesian_velocity_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t cartesian_velocity_command_value_from_view(const cartesian_velocity_command_t *view, cartesian_velocity_command_value_t *out);
wl_codec_status_t cartesian_velocity_command_value_to_view(const cartesian_velocity_command_value_t *value, cartesian_velocity_command_t *out);

void gripper_position_velocity_command_clear(gripper_position_velocity_command_t *value);
size_t gripper_position_velocity_command_encoded_size(const gripper_position_velocity_command_t *value);
wl_codec_status_t gripper_position_velocity_command_encode(const gripper_position_velocity_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t gripper_position_velocity_command_decode(const uint8_t *input, size_t input_length, gripper_position_velocity_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t gripper_position_velocity_command_value_from_view(const gripper_position_velocity_command_t *view, gripper_position_velocity_command_value_t *out);
wl_codec_status_t gripper_position_velocity_command_value_to_view(const gripper_position_velocity_command_value_t *value, gripper_position_velocity_command_t *out);

void gripper_velocity_command_clear(gripper_velocity_command_t *value);
size_t gripper_velocity_command_encoded_size(const gripper_velocity_command_t *value);
wl_codec_status_t gripper_velocity_command_encode(const gripper_velocity_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t gripper_velocity_command_decode(const uint8_t *input, size_t input_length, gripper_velocity_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t gripper_velocity_command_value_from_view(const gripper_velocity_command_t *view, gripper_velocity_command_value_t *out);
wl_codec_status_t gripper_velocity_command_value_to_view(const gripper_velocity_command_value_t *value, gripper_velocity_command_t *out);

void gripper_pvt_command_clear(gripper_pvt_command_t *value);
size_t gripper_pvt_command_encoded_size(const gripper_pvt_command_t *value);
wl_codec_status_t gripper_pvt_command_encode(const gripper_pvt_command_t *value, uint8_t *out, size_t out_capacity, size_t *out_length);
wl_codec_status_t gripper_pvt_command_decode(const uint8_t *input, size_t input_length, gripper_pvt_command_t *out);

/* Advanced conversions: views borrow value/input storage. Inputs and outputs
 * must not overlap; failures leave output unchanged. */
wl_codec_status_t gripper_pvt_command_value_from_view(const gripper_pvt_command_t *view, gripper_pvt_command_value_t *out);
wl_codec_status_t gripper_pvt_command_value_to_view(const gripper_pvt_command_value_t *value, gripper_pvt_command_t *out);

#ifdef __cplusplus
}
#endif

#endif
