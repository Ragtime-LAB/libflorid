#ifndef WIRELINK_GENERATED_FCI_DEVICE_H_BINDINGS
#define WIRELINK_GENERATED_FCI_DEVICE_H_BINDINGS

#include "fci_device.h"
#include <wirelink/link.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t fci_device_dispatch_domain_t;
enum {
  FCI_DEVICE_DISPATCH_OK = 0,
  FCI_DEVICE_DISPATCH_NON_RX,
  FCI_DEVICE_DISPATCH_UNKNOWN_MESSAGE,
  FCI_DEVICE_DISPATCH_MISSING_ROUTE,
  FCI_DEVICE_DISPATCH_MISSING_SCRATCH,
  FCI_DEVICE_DISPATCH_CODEC_ERROR,
  FCI_DEVICE_DISPATCH_HANDLER_ERROR,
  FCI_DEVICE_DISPATCH_INVALID_ARGUMENT
};

typedef struct {
  fci_device_dispatch_domain_t domain;
  uint16_t message_id;
  wl_event_type_t event_type;
  wl_codec_status_t codec_status;
  int32_t handler_result;
} fci_device_dispatch_result_t;

typedef struct {
  uint32_t delivered;
  uint32_t non_rx;
  uint32_t unknown_message;
  uint32_t missing_route;
  uint32_t missing_scratch;
  uint32_t codec_failure;
  uint32_t handler_failure;
} fci_device_dispatch_counters_t;

typedef int32_t fci_device_send_domain_t;
enum {
  FCI_DEVICE_SEND_OK = 0,
  FCI_DEVICE_SEND_CODEC_ERROR,
  FCI_DEVICE_SEND_CORE_ERROR
};

typedef struct {
  uint8_t *data;
  size_t capacity;
} fci_device_encode_scratch_t;

typedef struct {
  fci_device_send_domain_t domain;
  wl_codec_status_t codec_status;
  int core_result;
  size_t payload_length;
  wl_tx_handle_t handle;
} fci_device_send_result_t;

typedef int32_t (*fci_device_semantic_version_handler_fn)(void *user_data, const semantic_version_t *message, wl_delivery_t delivery);
typedef struct {
  semantic_version_t *scratch;
  fci_device_semantic_version_handler_fn handler;
  void *user_data;
} fci_device_semantic_version_route_t;

typedef int32_t (*fci_device_device_info_handler_fn)(void *user_data, const device_info_t *message, wl_delivery_t delivery);
typedef struct {
  device_info_t *scratch;
  fci_device_device_info_handler_fn handler;
  void *user_data;
} fci_device_device_info_route_t;

typedef int32_t (*fci_device_device_settings_handler_fn)(void *user_data, const device_settings_t *message, wl_delivery_t delivery);
typedef struct {
  device_settings_t *scratch;
  fci_device_device_settings_handler_fn handler;
  void *user_data;
} fci_device_device_settings_route_t;

typedef int32_t (*fci_device_arm_status_handler_fn)(void *user_data, const arm_status_t *message, wl_delivery_t delivery);
typedef struct {
  arm_status_t *scratch;
  fci_device_arm_status_handler_fn handler;
  void *user_data;
} fci_device_arm_status_route_t;

typedef int32_t (*fci_device_motor_feedback_handler_fn)(void *user_data, const motor_feedback_t *message, wl_delivery_t delivery);
typedef struct {
  motor_feedback_t *scratch;
  fci_device_motor_feedback_handler_fn handler;
  void *user_data;
} fci_device_motor_feedback_route_t;

typedef int32_t (*fci_device_arm_diagnostics_handler_fn)(void *user_data, const arm_diagnostics_t *message, wl_delivery_t delivery);
typedef struct {
  arm_diagnostics_t *scratch;
  fci_device_arm_diagnostics_handler_fn handler;
  void *user_data;
} fci_device_arm_diagnostics_route_t;

typedef int32_t (*fci_device_set_zero_request_handler_fn)(void *user_data, const set_zero_request_t *message, wl_delivery_t delivery);
typedef struct {
  set_zero_request_t *scratch;
  fci_device_set_zero_request_handler_fn handler;
  void *user_data;
} fci_device_set_zero_request_route_t;

typedef int32_t (*fci_device_set_zero_response_handler_fn)(void *user_data, const set_zero_response_t *message, wl_delivery_t delivery);
typedef struct {
  set_zero_response_t *scratch;
  fci_device_set_zero_response_handler_fn handler;
  void *user_data;
} fci_device_set_zero_response_route_t;

typedef int32_t (*fci_device_clear_error_request_handler_fn)(void *user_data, const clear_error_request_t *message, wl_delivery_t delivery);
typedef struct {
  clear_error_request_t *scratch;
  fci_device_clear_error_request_handler_fn handler;
  void *user_data;
} fci_device_clear_error_request_route_t;

typedef int32_t (*fci_device_clear_error_response_handler_fn)(void *user_data, const clear_error_response_t *message, wl_delivery_t delivery);
typedef struct {
  clear_error_response_t *scratch;
  fci_device_clear_error_response_handler_fn handler;
  void *user_data;
} fci_device_clear_error_response_route_t;

typedef int32_t (*fci_device_home_request_handler_fn)(void *user_data, const home_request_t *message, wl_delivery_t delivery);
typedef struct {
  home_request_t *scratch;
  fci_device_home_request_handler_fn handler;
  void *user_data;
} fci_device_home_request_route_t;

typedef int32_t (*fci_device_home_response_handler_fn)(void *user_data, const home_response_t *message, wl_delivery_t delivery);
typedef struct {
  home_response_t *scratch;
  fci_device_home_response_handler_fn handler;
  void *user_data;
} fci_device_home_response_route_t;

typedef int32_t (*fci_device_clear_faults_request_handler_fn)(void *user_data, const clear_faults_request_t *message, wl_delivery_t delivery);
typedef struct {
  clear_faults_request_t *scratch;
  fci_device_clear_faults_request_handler_fn handler;
  void *user_data;
} fci_device_clear_faults_request_route_t;

typedef int32_t (*fci_device_clear_faults_response_handler_fn)(void *user_data, const clear_faults_response_t *message, wl_delivery_t delivery);
typedef struct {
  clear_faults_response_t *scratch;
  fci_device_clear_faults_response_handler_fn handler;
  void *user_data;
} fci_device_clear_faults_response_route_t;

typedef int32_t (*fci_device_acquire_control_lease_request_handler_fn)(void *user_data, const acquire_control_lease_request_t *message, wl_delivery_t delivery);
typedef struct {
  acquire_control_lease_request_t *scratch;
  fci_device_acquire_control_lease_request_handler_fn handler;
  void *user_data;
} fci_device_acquire_control_lease_request_route_t;

typedef int32_t (*fci_device_acquire_control_lease_response_handler_fn)(void *user_data, const acquire_control_lease_response_t *message, wl_delivery_t delivery);
typedef struct {
  acquire_control_lease_response_t *scratch;
  fci_device_acquire_control_lease_response_handler_fn handler;
  void *user_data;
} fci_device_acquire_control_lease_response_route_t;

typedef int32_t (*fci_device_release_control_lease_request_handler_fn)(void *user_data, const release_control_lease_request_t *message, wl_delivery_t delivery);
typedef struct {
  release_control_lease_request_t *scratch;
  fci_device_release_control_lease_request_handler_fn handler;
  void *user_data;
} fci_device_release_control_lease_request_route_t;

typedef int32_t (*fci_device_release_control_lease_response_handler_fn)(void *user_data, const release_control_lease_response_t *message, wl_delivery_t delivery);
typedef struct {
  release_control_lease_response_t *scratch;
  fci_device_release_control_lease_response_handler_fn handler;
  void *user_data;
} fci_device_release_control_lease_response_route_t;

typedef int32_t (*fci_device_get_motor_feedback_request_handler_fn)(void *user_data, const get_motor_feedback_request_t *message, wl_delivery_t delivery);
typedef struct {
  get_motor_feedback_request_t *scratch;
  fci_device_get_motor_feedback_request_handler_fn handler;
  void *user_data;
} fci_device_get_motor_feedback_request_route_t;

typedef int32_t (*fci_device_get_motor_feedback_response_handler_fn)(void *user_data, const get_motor_feedback_response_t *message, wl_delivery_t delivery);
typedef struct {
  get_motor_feedback_response_t *scratch;
  fci_device_get_motor_feedback_response_handler_fn handler;
  void *user_data;
} fci_device_get_motor_feedback_response_route_t;

typedef int32_t (*fci_device_get_device_info_request_handler_fn)(void *user_data, const get_device_info_request_t *message, wl_delivery_t delivery);
typedef struct {
  get_device_info_request_t *scratch;
  fci_device_get_device_info_request_handler_fn handler;
  void *user_data;
} fci_device_get_device_info_request_route_t;

typedef int32_t (*fci_device_get_device_info_response_handler_fn)(void *user_data, const get_device_info_response_t *message, wl_delivery_t delivery);
typedef struct {
  get_device_info_response_t *scratch;
  fci_device_get_device_info_response_handler_fn handler;
  void *user_data;
} fci_device_get_device_info_response_route_t;

typedef int32_t (*fci_device_set_device_info_request_handler_fn)(void *user_data, const set_device_info_request_t *message, wl_delivery_t delivery);
typedef struct {
  set_device_info_request_t *scratch;
  fci_device_set_device_info_request_handler_fn handler;
  void *user_data;
} fci_device_set_device_info_request_route_t;

typedef int32_t (*fci_device_set_device_info_response_handler_fn)(void *user_data, const set_device_info_response_t *message, wl_delivery_t delivery);
typedef struct {
  set_device_info_response_t *scratch;
  fci_device_set_device_info_response_handler_fn handler;
  void *user_data;
} fci_device_set_device_info_response_route_t;

typedef int32_t (*fci_device_set_arm_control_mode_request_handler_fn)(void *user_data, const set_arm_control_mode_request_t *message, wl_delivery_t delivery);
typedef struct {
  set_arm_control_mode_request_t *scratch;
  fci_device_set_arm_control_mode_request_handler_fn handler;
  void *user_data;
} fci_device_set_arm_control_mode_request_route_t;

typedef int32_t (*fci_device_set_arm_control_mode_response_handler_fn)(void *user_data, const set_arm_control_mode_response_t *message, wl_delivery_t delivery);
typedef struct {
  set_arm_control_mode_response_t *scratch;
  fci_device_set_arm_control_mode_response_handler_fn handler;
  void *user_data;
} fci_device_set_arm_control_mode_response_route_t;

typedef int32_t (*fci_device_set_gripper_control_mode_request_handler_fn)(void *user_data, const set_gripper_control_mode_request_t *message, wl_delivery_t delivery);
typedef struct {
  set_gripper_control_mode_request_t *scratch;
  fci_device_set_gripper_control_mode_request_handler_fn handler;
  void *user_data;
} fci_device_set_gripper_control_mode_request_route_t;

typedef int32_t (*fci_device_set_gripper_control_mode_response_handler_fn)(void *user_data, const set_gripper_control_mode_response_t *message, wl_delivery_t delivery);
typedef struct {
  set_gripper_control_mode_response_t *scratch;
  fci_device_set_gripper_control_mode_response_handler_fn handler;
  void *user_data;
} fci_device_set_gripper_control_mode_response_route_t;

typedef int32_t (*fci_device_motor_register_read_request_handler_fn)(void *user_data, const motor_register_read_request_t *message, wl_delivery_t delivery);
typedef struct {
  motor_register_read_request_t *scratch;
  fci_device_motor_register_read_request_handler_fn handler;
  void *user_data;
} fci_device_motor_register_read_request_route_t;

typedef int32_t (*fci_device_motor_register_read_response_handler_fn)(void *user_data, const motor_register_read_response_t *message, wl_delivery_t delivery);
typedef struct {
  motor_register_read_response_t *scratch;
  fci_device_motor_register_read_response_handler_fn handler;
  void *user_data;
} fci_device_motor_register_read_response_route_t;

typedef int32_t (*fci_device_motor_register_write_request_handler_fn)(void *user_data, const motor_register_write_request_t *message, wl_delivery_t delivery);
typedef struct {
  motor_register_write_request_t *scratch;
  fci_device_motor_register_write_request_handler_fn handler;
  void *user_data;
} fci_device_motor_register_write_request_route_t;

typedef int32_t (*fci_device_motor_register_write_response_handler_fn)(void *user_data, const motor_register_write_response_t *message, wl_delivery_t delivery);
typedef struct {
  motor_register_write_response_t *scratch;
  fci_device_motor_register_write_response_handler_fn handler;
  void *user_data;
} fci_device_motor_register_write_response_route_t;

typedef int32_t (*fci_device_motor_store_parameters_request_handler_fn)(void *user_data, const motor_store_parameters_request_t *message, wl_delivery_t delivery);
typedef struct {
  motor_store_parameters_request_t *scratch;
  fci_device_motor_store_parameters_request_handler_fn handler;
  void *user_data;
} fci_device_motor_store_parameters_request_route_t;

typedef int32_t (*fci_device_motor_store_parameters_response_handler_fn)(void *user_data, const motor_store_parameters_response_t *message, wl_delivery_t delivery);
typedef struct {
  motor_store_parameters_response_t *scratch;
  fci_device_motor_store_parameters_response_handler_fn handler;
  void *user_data;
} fci_device_motor_store_parameters_response_route_t;

typedef int32_t (*fci_device_motor_set_zero_request_handler_fn)(void *user_data, const motor_set_zero_request_t *message, wl_delivery_t delivery);
typedef struct {
  motor_set_zero_request_t *scratch;
  fci_device_motor_set_zero_request_handler_fn handler;
  void *user_data;
} fci_device_motor_set_zero_request_route_t;

typedef int32_t (*fci_device_motor_set_zero_response_handler_fn)(void *user_data, const motor_set_zero_response_t *message, wl_delivery_t delivery);
typedef struct {
  motor_set_zero_response_t *scratch;
  fci_device_motor_set_zero_response_handler_fn handler;
  void *user_data;
} fci_device_motor_set_zero_response_route_t;

typedef int32_t (*fci_device_set_arm_mode_request_handler_fn)(void *user_data, const set_arm_mode_request_t *message, wl_delivery_t delivery);
typedef struct {
  set_arm_mode_request_t *scratch;
  fci_device_set_arm_mode_request_handler_fn handler;
  void *user_data;
} fci_device_set_arm_mode_request_route_t;

typedef int32_t (*fci_device_set_arm_mode_response_handler_fn)(void *user_data, const set_arm_mode_response_t *message, wl_delivery_t delivery);
typedef struct {
  set_arm_mode_response_t *scratch;
  fci_device_set_arm_mode_response_handler_fn handler;
  void *user_data;
} fci_device_set_arm_mode_response_route_t;

typedef int32_t (*fci_device_get_device_settings_request_handler_fn)(void *user_data, const get_device_settings_request_t *message, wl_delivery_t delivery);
typedef struct {
  get_device_settings_request_t *scratch;
  fci_device_get_device_settings_request_handler_fn handler;
  void *user_data;
} fci_device_get_device_settings_request_route_t;

typedef int32_t (*fci_device_get_device_settings_response_handler_fn)(void *user_data, const get_device_settings_response_t *message, wl_delivery_t delivery);
typedef struct {
  get_device_settings_response_t *scratch;
  fci_device_get_device_settings_response_handler_fn handler;
  void *user_data;
} fci_device_get_device_settings_response_route_t;

typedef int32_t (*fci_device_set_device_settings_request_handler_fn)(void *user_data, const set_device_settings_request_t *message, wl_delivery_t delivery);
typedef struct {
  set_device_settings_request_t *scratch;
  fci_device_set_device_settings_request_handler_fn handler;
  void *user_data;
} fci_device_set_device_settings_request_route_t;

typedef int32_t (*fci_device_set_device_settings_response_handler_fn)(void *user_data, const set_device_settings_response_t *message, wl_delivery_t delivery);
typedef struct {
  set_device_settings_response_t *scratch;
  fci_device_set_device_settings_response_handler_fn handler;
  void *user_data;
} fci_device_set_device_settings_response_route_t;

typedef int32_t (*fci_device_joint_mit_command_handler_fn)(void *user_data, const joint_mit_command_t *message, wl_delivery_t delivery);
typedef struct {
  joint_mit_command_t *scratch;
  fci_device_joint_mit_command_handler_fn handler;
  void *user_data;
} fci_device_joint_mit_command_route_t;

typedef int32_t (*fci_device_emergency_stop_request_handler_fn)(void *user_data, const emergency_stop_request_t *message, wl_delivery_t delivery);
typedef struct {
  emergency_stop_request_t *scratch;
  fci_device_emergency_stop_request_handler_fn handler;
  void *user_data;
} fci_device_emergency_stop_request_route_t;

typedef int32_t (*fci_device_emergency_stop_response_handler_fn)(void *user_data, const emergency_stop_response_t *message, wl_delivery_t delivery);
typedef struct {
  emergency_stop_response_t *scratch;
  fci_device_emergency_stop_response_handler_fn handler;
  void *user_data;
} fci_device_emergency_stop_response_route_t;

typedef int32_t (*fci_device_gripper_mit_command_handler_fn)(void *user_data, const gripper_mit_command_t *message, wl_delivery_t delivery);
typedef struct {
  gripper_mit_command_t *scratch;
  fci_device_gripper_mit_command_handler_fn handler;
  void *user_data;
} fci_device_gripper_mit_command_route_t;

typedef int32_t (*fci_device_joint_position_velocity_command_handler_fn)(void *user_data, const joint_position_velocity_command_t *message, wl_delivery_t delivery);
typedef struct {
  joint_position_velocity_command_t *scratch;
  fci_device_joint_position_velocity_command_handler_fn handler;
  void *user_data;
} fci_device_joint_position_velocity_command_route_t;

typedef int32_t (*fci_device_joint_velocity_command_handler_fn)(void *user_data, const joint_velocity_command_t *message, wl_delivery_t delivery);
typedef struct {
  joint_velocity_command_t *scratch;
  fci_device_joint_velocity_command_handler_fn handler;
  void *user_data;
} fci_device_joint_velocity_command_route_t;

typedef int32_t (*fci_device_joint_pvt_command_handler_fn)(void *user_data, const joint_pvt_command_t *message, wl_delivery_t delivery);
typedef struct {
  joint_pvt_command_t *scratch;
  fci_device_joint_pvt_command_handler_fn handler;
  void *user_data;
} fci_device_joint_pvt_command_route_t;

typedef int32_t (*fci_device_cartesian_pose_command_handler_fn)(void *user_data, const cartesian_pose_command_t *message, wl_delivery_t delivery);
typedef struct {
  cartesian_pose_command_t *scratch;
  fci_device_cartesian_pose_command_handler_fn handler;
  void *user_data;
} fci_device_cartesian_pose_command_route_t;

typedef int32_t (*fci_device_cartesian_velocity_command_handler_fn)(void *user_data, const cartesian_velocity_command_t *message, wl_delivery_t delivery);
typedef struct {
  cartesian_velocity_command_t *scratch;
  fci_device_cartesian_velocity_command_handler_fn handler;
  void *user_data;
} fci_device_cartesian_velocity_command_route_t;

typedef int32_t (*fci_device_gripper_position_velocity_command_handler_fn)(void *user_data, const gripper_position_velocity_command_t *message, wl_delivery_t delivery);
typedef struct {
  gripper_position_velocity_command_t *scratch;
  fci_device_gripper_position_velocity_command_handler_fn handler;
  void *user_data;
} fci_device_gripper_position_velocity_command_route_t;

typedef int32_t (*fci_device_gripper_velocity_command_handler_fn)(void *user_data, const gripper_velocity_command_t *message, wl_delivery_t delivery);
typedef struct {
  gripper_velocity_command_t *scratch;
  fci_device_gripper_velocity_command_handler_fn handler;
  void *user_data;
} fci_device_gripper_velocity_command_route_t;

typedef int32_t (*fci_device_gripper_pvt_command_handler_fn)(void *user_data, const gripper_pvt_command_t *message, wl_delivery_t delivery);
typedef struct {
  gripper_pvt_command_t *scratch;
  fci_device_gripper_pvt_command_handler_fn handler;
  void *user_data;
} fci_device_gripper_pvt_command_route_t;

typedef int32_t (*fci_device_image_version_handler_fn)(void *user_data, const image_version_t *message, wl_delivery_t delivery);
typedef struct {
  image_version_t *scratch;
  fci_device_image_version_handler_fn handler;
  void *user_data;
} fci_device_image_version_route_t;

typedef int32_t (*fci_device_boot_status_info_handler_fn)(void *user_data, const boot_status_info_t *message, wl_delivery_t delivery);
typedef struct {
  boot_status_info_t *scratch;
  fci_device_boot_status_info_handler_fn handler;
  void *user_data;
} fci_device_boot_status_info_route_t;

typedef int32_t (*fci_device_bulk_begin_handler_fn)(void *user_data, const bulk_begin_t *message, wl_delivery_t delivery);
typedef struct {
  bulk_begin_t *scratch;
  fci_device_bulk_begin_handler_fn handler;
  void *user_data;
} fci_device_bulk_begin_route_t;

typedef int32_t (*fci_device_bulk_chunk_handler_fn)(void *user_data, const bulk_chunk_t *message, wl_delivery_t delivery);
typedef struct {
  bulk_chunk_t *scratch;
  fci_device_bulk_chunk_handler_fn handler;
  void *user_data;
} fci_device_bulk_chunk_route_t;

typedef int32_t (*fci_device_bulk_end_handler_fn)(void *user_data, const bulk_end_t *message, wl_delivery_t delivery);
typedef struct {
  bulk_end_t *scratch;
  fci_device_bulk_end_handler_fn handler;
  void *user_data;
} fci_device_bulk_end_route_t;

typedef int32_t (*fci_device_bulk_abort_handler_fn)(void *user_data, const bulk_abort_t *message, wl_delivery_t delivery);
typedef struct {
  bulk_abort_t *scratch;
  fci_device_bulk_abort_handler_fn handler;
  void *user_data;
} fci_device_bulk_abort_route_t;

typedef int32_t (*fci_device_bulk_status_handler_fn)(void *user_data, const bulk_status_t *message, wl_delivery_t delivery);
typedef struct {
  bulk_status_t *scratch;
  fci_device_bulk_status_handler_fn handler;
  void *user_data;
} fci_device_bulk_status_route_t;

typedef int32_t (*fci_device_get_boot_status_request_handler_fn)(void *user_data, const get_boot_status_request_t *message, wl_delivery_t delivery);
typedef struct {
  get_boot_status_request_t *scratch;
  fci_device_get_boot_status_request_handler_fn handler;
  void *user_data;
} fci_device_get_boot_status_request_route_t;

typedef int32_t (*fci_device_get_boot_status_response_handler_fn)(void *user_data, const get_boot_status_response_t *message, wl_delivery_t delivery);
typedef struct {
  get_boot_status_response_t *scratch;
  fci_device_get_boot_status_response_handler_fn handler;
  void *user_data;
} fci_device_get_boot_status_response_route_t;

typedef int32_t (*fci_device_start_upgrade_request_handler_fn)(void *user_data, const start_upgrade_request_t *message, wl_delivery_t delivery);
typedef struct {
  start_upgrade_request_t *scratch;
  fci_device_start_upgrade_request_handler_fn handler;
  void *user_data;
} fci_device_start_upgrade_request_route_t;

typedef int32_t (*fci_device_start_upgrade_response_handler_fn)(void *user_data, const start_upgrade_response_t *message, wl_delivery_t delivery);
typedef struct {
  start_upgrade_response_t *scratch;
  fci_device_start_upgrade_response_handler_fn handler;
  void *user_data;
} fci_device_start_upgrade_response_route_t;

typedef int32_t (*fci_device_reboot_request_handler_fn)(void *user_data, const reboot_request_t *message, wl_delivery_t delivery);
typedef struct {
  reboot_request_t *scratch;
  fci_device_reboot_request_handler_fn handler;
  void *user_data;
} fci_device_reboot_request_route_t;

typedef int32_t (*fci_device_reboot_response_handler_fn)(void *user_data, const reboot_response_t *message, wl_delivery_t delivery);
typedef struct {
  reboot_response_t *scratch;
  fci_device_reboot_response_handler_fn handler;
  void *user_data;
} fci_device_reboot_response_route_t;

typedef struct {
  fci_device_semantic_version_route_t semantic_version;
  fci_device_device_info_route_t device_info;
  fci_device_device_settings_route_t device_settings;
  fci_device_arm_status_route_t arm_status;
  fci_device_motor_feedback_route_t motor_feedback;
  fci_device_arm_diagnostics_route_t arm_diagnostics;
  fci_device_set_zero_request_route_t set_zero_request;
  fci_device_set_zero_response_route_t set_zero_response;
  fci_device_clear_error_request_route_t clear_error_request;
  fci_device_clear_error_response_route_t clear_error_response;
  fci_device_home_request_route_t home_request;
  fci_device_home_response_route_t home_response;
  fci_device_clear_faults_request_route_t clear_faults_request;
  fci_device_clear_faults_response_route_t clear_faults_response;
  fci_device_acquire_control_lease_request_route_t acquire_control_lease_request;
  fci_device_acquire_control_lease_response_route_t acquire_control_lease_response;
  fci_device_release_control_lease_request_route_t release_control_lease_request;
  fci_device_release_control_lease_response_route_t release_control_lease_response;
  fci_device_get_motor_feedback_request_route_t get_motor_feedback_request;
  fci_device_get_motor_feedback_response_route_t get_motor_feedback_response;
  fci_device_get_device_info_request_route_t get_device_info_request;
  fci_device_get_device_info_response_route_t get_device_info_response;
  fci_device_set_device_info_request_route_t set_device_info_request;
  fci_device_set_device_info_response_route_t set_device_info_response;
  fci_device_set_arm_control_mode_request_route_t set_arm_control_mode_request;
  fci_device_set_arm_control_mode_response_route_t set_arm_control_mode_response;
  fci_device_set_gripper_control_mode_request_route_t set_gripper_control_mode_request;
  fci_device_set_gripper_control_mode_response_route_t set_gripper_control_mode_response;
  fci_device_motor_register_read_request_route_t motor_register_read_request;
  fci_device_motor_register_read_response_route_t motor_register_read_response;
  fci_device_motor_register_write_request_route_t motor_register_write_request;
  fci_device_motor_register_write_response_route_t motor_register_write_response;
  fci_device_motor_store_parameters_request_route_t motor_store_parameters_request;
  fci_device_motor_store_parameters_response_route_t motor_store_parameters_response;
  fci_device_motor_set_zero_request_route_t motor_set_zero_request;
  fci_device_motor_set_zero_response_route_t motor_set_zero_response;
  fci_device_set_arm_mode_request_route_t set_arm_mode_request;
  fci_device_set_arm_mode_response_route_t set_arm_mode_response;
  fci_device_get_device_settings_request_route_t get_device_settings_request;
  fci_device_get_device_settings_response_route_t get_device_settings_response;
  fci_device_set_device_settings_request_route_t set_device_settings_request;
  fci_device_set_device_settings_response_route_t set_device_settings_response;
  fci_device_joint_mit_command_route_t joint_mit_command;
  fci_device_emergency_stop_request_route_t emergency_stop_request;
  fci_device_emergency_stop_response_route_t emergency_stop_response;
  fci_device_gripper_mit_command_route_t gripper_mit_command;
  fci_device_joint_position_velocity_command_route_t joint_position_velocity_command;
  fci_device_joint_velocity_command_route_t joint_velocity_command;
  fci_device_joint_pvt_command_route_t joint_pvt_command;
  fci_device_cartesian_pose_command_route_t cartesian_pose_command;
  fci_device_cartesian_velocity_command_route_t cartesian_velocity_command;
  fci_device_gripper_position_velocity_command_route_t gripper_position_velocity_command;
  fci_device_gripper_velocity_command_route_t gripper_velocity_command;
  fci_device_gripper_pvt_command_route_t gripper_pvt_command;
  fci_device_image_version_route_t image_version;
  fci_device_boot_status_info_route_t boot_status_info;
  fci_device_bulk_begin_route_t bulk_begin;
  fci_device_bulk_chunk_route_t bulk_chunk;
  fci_device_bulk_end_route_t bulk_end;
  fci_device_bulk_abort_route_t bulk_abort;
  fci_device_bulk_status_route_t bulk_status;
  fci_device_get_boot_status_request_route_t get_boot_status_request;
  fci_device_get_boot_status_response_route_t get_boot_status_response;
  fci_device_start_upgrade_request_route_t start_upgrade_request;
  fci_device_start_upgrade_response_route_t start_upgrade_response;
  fci_device_reboot_request_route_t reboot_request;
  fci_device_reboot_response_route_t reboot_response;
  fci_device_dispatch_counters_t counters;
} fci_device_router_t;

fci_device_dispatch_result_t fci_device_dispatch_event(wl_ctx_t *ctx, const wl_event_t *event, fci_device_router_t *router);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_semantic_version_send(wl_ctx_t *ctx, const semantic_version_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_device_info_send(wl_ctx_t *ctx, const device_info_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_device_settings_send(wl_ctx_t *ctx, const device_settings_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_arm_status_send(wl_ctx_t *ctx, const arm_status_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_feedback_send(wl_ctx_t *ctx, const motor_feedback_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_arm_diagnostics_send(wl_ctx_t *ctx, const arm_diagnostics_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_zero_request_send(wl_ctx_t *ctx, const set_zero_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_zero_response_send(wl_ctx_t *ctx, const set_zero_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_clear_error_request_send(wl_ctx_t *ctx, const clear_error_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_clear_error_response_send(wl_ctx_t *ctx, const clear_error_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_home_request_send(wl_ctx_t *ctx, const home_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_home_response_send(wl_ctx_t *ctx, const home_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_clear_faults_request_send(wl_ctx_t *ctx, const clear_faults_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_clear_faults_response_send(wl_ctx_t *ctx, const clear_faults_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_acquire_control_lease_request_send(wl_ctx_t *ctx, const acquire_control_lease_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_acquire_control_lease_response_send(wl_ctx_t *ctx, const acquire_control_lease_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_release_control_lease_request_send(wl_ctx_t *ctx, const release_control_lease_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_release_control_lease_response_send(wl_ctx_t *ctx, const release_control_lease_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_motor_feedback_request_send(wl_ctx_t *ctx, const get_motor_feedback_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_motor_feedback_response_send(wl_ctx_t *ctx, const get_motor_feedback_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_device_info_request_send(wl_ctx_t *ctx, const get_device_info_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_device_info_response_send(wl_ctx_t *ctx, const get_device_info_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_device_info_request_send(wl_ctx_t *ctx, const set_device_info_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_device_info_response_send(wl_ctx_t *ctx, const set_device_info_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_arm_control_mode_request_send(wl_ctx_t *ctx, const set_arm_control_mode_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_arm_control_mode_response_send(wl_ctx_t *ctx, const set_arm_control_mode_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_gripper_control_mode_request_send(wl_ctx_t *ctx, const set_gripper_control_mode_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_gripper_control_mode_response_send(wl_ctx_t *ctx, const set_gripper_control_mode_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_register_read_request_send(wl_ctx_t *ctx, const motor_register_read_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_register_read_response_send(wl_ctx_t *ctx, const motor_register_read_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_register_write_request_send(wl_ctx_t *ctx, const motor_register_write_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_register_write_response_send(wl_ctx_t *ctx, const motor_register_write_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_store_parameters_request_send(wl_ctx_t *ctx, const motor_store_parameters_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_store_parameters_response_send(wl_ctx_t *ctx, const motor_store_parameters_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_set_zero_request_send(wl_ctx_t *ctx, const motor_set_zero_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_motor_set_zero_response_send(wl_ctx_t *ctx, const motor_set_zero_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_arm_mode_request_send(wl_ctx_t *ctx, const set_arm_mode_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_arm_mode_response_send(wl_ctx_t *ctx, const set_arm_mode_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_device_settings_request_send(wl_ctx_t *ctx, const get_device_settings_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_device_settings_response_send(wl_ctx_t *ctx, const get_device_settings_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_device_settings_request_send(wl_ctx_t *ctx, const set_device_settings_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_set_device_settings_response_send(wl_ctx_t *ctx, const set_device_settings_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_joint_mit_command_send(wl_ctx_t *ctx, const joint_mit_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_emergency_stop_request_send(wl_ctx_t *ctx, const emergency_stop_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_emergency_stop_response_send(wl_ctx_t *ctx, const emergency_stop_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_gripper_mit_command_send(wl_ctx_t *ctx, const gripper_mit_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_joint_position_velocity_command_send(wl_ctx_t *ctx, const joint_position_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_joint_velocity_command_send(wl_ctx_t *ctx, const joint_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_joint_pvt_command_send(wl_ctx_t *ctx, const joint_pvt_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_cartesian_pose_command_send(wl_ctx_t *ctx, const cartesian_pose_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_cartesian_velocity_command_send(wl_ctx_t *ctx, const cartesian_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_gripper_position_velocity_command_send(wl_ctx_t *ctx, const gripper_position_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_gripper_velocity_command_send(wl_ctx_t *ctx, const gripper_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_gripper_pvt_command_send(wl_ctx_t *ctx, const gripper_pvt_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_image_version_send(wl_ctx_t *ctx, const image_version_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_boot_status_info_send(wl_ctx_t *ctx, const boot_status_info_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_bulk_begin_send(wl_ctx_t *ctx, const bulk_begin_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_bulk_chunk_send(wl_ctx_t *ctx, const bulk_chunk_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_bulk_end_send(wl_ctx_t *ctx, const bulk_end_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_bulk_abort_send(wl_ctx_t *ctx, const bulk_abort_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_bulk_status_send(wl_ctx_t *ctx, const bulk_status_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_boot_status_request_send(wl_ctx_t *ctx, const get_boot_status_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_get_boot_status_response_send(wl_ctx_t *ctx, const get_boot_status_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_start_upgrade_request_send(wl_ctx_t *ctx, const start_upgrade_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_start_upgrade_response_send(wl_ctx_t *ctx, const start_upgrade_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_reboot_request_send(wl_ctx_t *ctx, const reboot_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

/* Encodes directly into Wirelink-owned TX storage. */
fci_device_send_result_t fci_device_reboot_response_send(wl_ctx_t *ctx, const reboot_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
