#ifndef WIRELINK_GENERATED_FCI_ARM_RUNTIME_H
#define WIRELINK_GENERATED_FCI_ARM_RUNTIME_H

#include "fci_arm_bindings.h"
#include <wirelink/pump.h>
#include <wirelink/endpoint.h>
#include <wirelink/allocator.h>
#include <wirelink/frame.h>
#include <string.h>
#include <wirelink/latest.h>
#include <wirelink/rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FCI_ARM_SCHEMA_IDENTITY UINT64_C(0x7B0F695D2A4ABFB3)
#define FCI_ARM_BINDING_PROFILE_IDENTITY UINT64_C(0x73973E42BF90BF73)
#define FCI_ARM_BINDING_PROFILE_VERSION 1U
#define FCI_ARM_IDENTITY_ALGORITHM "fnv1a64-v1"

#define FCI_ARM_RUNTIME_CODEGEN_ABI_VERSION 30U

/* Generated capabilities, not application overrides. */
#define FCI_ARM_RUNTIME_HAS_RPC_CLIENT 1
#define FCI_ARM_RUNTIME_HAS_RPC_SERVER 1

#define FCI_ARM_RPC_REQUEST_FINGERPRINT_ALGORITHM "fnv1a64-canonical-request-v1"

typedef int32_t fci_arm_runtime_domain_t;
enum {
  FCI_ARM_RUNTIME_OK = 0,
  FCI_ARM_RUNTIME_NON_RX,
  FCI_ARM_RUNTIME_UNKNOWN_MESSAGE,
  FCI_ARM_RUNTIME_MISSING_ROUTE,
  FCI_ARM_RUNTIME_MISSING_SCRATCH,
  FCI_ARM_RUNTIME_DELIVERY_MISMATCH,
  FCI_ARM_RUNTIME_CODEC_ERROR,
  FCI_ARM_RUNTIME_STORAGE_ERROR,
  FCI_ARM_RUNTIME_RPC_ERROR,
  FCI_ARM_RUNTIME_CORE_ERROR,
  FCI_ARM_RUNTIME_APPLICATION_ERROR,
  FCI_ARM_RUNTIME_INVALID_ARGUMENT
};

typedef uint8_t fci_arm_runtime_detail_kind_t;
enum {
  FCI_ARM_RUNTIME_DETAIL_NONE = 0,
  FCI_ARM_RUNTIME_DETAIL_RETAINED = 1,
  FCI_ARM_RUNTIME_DETAIL_RPC = 2
};

typedef struct {
  wl_codec_status_t codec_status;
  int32_t storage_result;
  int32_t abort_result;
} fci_arm_runtime_retained_detail_t;

typedef struct {
  wl_codec_status_t codec_status;
  wl_rpc_err_t rpc_result;
  int32_t core_result;
  int32_t application_result;
  wl_rpc_server_disposition_t rpc_disposition;
  uint32_t operation_id;
  wl_tx_handle_t handle;
  uint8_t peer_changed;
  size_t payload_length;
  union {
    wl_rpc_server_request_t server_request;
    wl_rpc_server_response_t server_response;
  };
} fci_arm_runtime_rpc_detail_t;

typedef union {
  fci_arm_runtime_retained_detail_t retained;
  fci_arm_runtime_rpc_detail_t rpc;
} fci_arm_runtime_detail_t;

/* Inspect detail only through the member selected by detail_kind. domain
 * classifies the outcome; zero-initialized unused detail fields retain their
 * corresponding success values. event_consumed is nonzero only when dispatch
 * released an RX event or reclaimed a terminal TX handle. */
typedef struct {
  fci_arm_runtime_domain_t domain;
  wl_event_type_t event_type;
  uint16_t message_id;
  fci_arm_runtime_detail_kind_t detail_kind;
  uint8_t event_consumed;
  fci_arm_runtime_detail_t detail;
} fci_arm_runtime_result_t;

/* Convenience helpers preserve the full diagnostic result. Detail accessors
 * return null unless detail_kind selects the requested member. Result strings
 * are diagnostic text and must not be parsed as a stable machine interface. */
static inline bool fci_arm_runtime_result_ok(const fci_arm_runtime_result_t *result) {
  return result != NULL && result->domain == FCI_ARM_RUNTIME_OK;
}

const char *fci_arm_runtime_result_str(const fci_arm_runtime_result_t *result);

static inline const fci_arm_runtime_retained_detail_t *fci_arm_runtime_result_retained_detail(const fci_arm_runtime_result_t *result) {
  return result != NULL && result->detail_kind == FCI_ARM_RUNTIME_DETAIL_RETAINED ? &result->detail.retained : NULL;
}

static inline const fci_arm_runtime_rpc_detail_t *fci_arm_runtime_result_rpc_detail(const fci_arm_runtime_result_t *result) {
  return result != NULL && result->detail_kind == FCI_ARM_RUNTIME_DETAIL_RPC ? &result->detail.rpc : NULL;
}

#define FCI_ARM_RUNTIME_HAS_MANAGED_RPC 0
/* The typed value remains borrowed until the matching release. */
typedef struct {
  const arm_status_t *value;
  uint32_t generation;
  wl_latest_view_t lease;
} fci_arm_arm_status_latest_view_t;

/* The typed value remains borrowed until the matching release. */
typedef struct {
  const motor_feedback_t *value;
  uint32_t generation;
  wl_latest_view_t lease;
} fci_arm_motor_feedback_latest_view_t;

/* The typed value remains borrowed until the matching release. */
typedef struct {
  const arm_diagnostics_t *value;
  uint32_t generation;
  wl_latest_view_t lease;
} fci_arm_arm_diagnostics_latest_view_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_acquire_control_lease_rpc_request_handler_fn)(void *user_data, const acquire_control_lease_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  acquire_control_lease_request_t *request_scratch;
  acquire_control_lease_response_t *response_scratch;
  fci_arm_acquire_control_lease_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_acquire_control_lease_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_clear_error_rpc_request_handler_fn)(void *user_data, const clear_error_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  clear_error_request_t *request_scratch;
  clear_error_response_t *response_scratch;
  fci_arm_clear_error_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_clear_error_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_clear_faults_rpc_request_handler_fn)(void *user_data, const clear_faults_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  clear_faults_request_t *request_scratch;
  clear_faults_response_t *response_scratch;
  fci_arm_clear_faults_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_clear_faults_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_emergency_stop_rpc_request_handler_fn)(void *user_data, const emergency_stop_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  emergency_stop_request_t *request_scratch;
  emergency_stop_response_t *response_scratch;
  fci_arm_emergency_stop_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_emergency_stop_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_get_device_info_rpc_request_handler_fn)(void *user_data, const get_device_info_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  get_device_info_request_t *request_scratch;
  get_device_info_response_t *response_scratch;
  fci_arm_get_device_info_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_get_device_info_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_get_device_settings_rpc_request_handler_fn)(void *user_data, const get_device_settings_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  get_device_settings_request_t *request_scratch;
  get_device_settings_response_t *response_scratch;
  fci_arm_get_device_settings_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_get_device_settings_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_get_motor_feedback_rpc_request_handler_fn)(void *user_data, const get_motor_feedback_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  get_motor_feedback_request_t *request_scratch;
  get_motor_feedback_response_t *response_scratch;
  fci_arm_get_motor_feedback_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_get_motor_feedback_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_home_rpc_request_handler_fn)(void *user_data, const home_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  home_request_t *request_scratch;
  home_response_t *response_scratch;
  fci_arm_home_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_home_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_motor_register_read_rpc_request_handler_fn)(void *user_data, const motor_register_read_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  motor_register_read_request_t *request_scratch;
  motor_register_read_response_t *response_scratch;
  fci_arm_motor_register_read_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_motor_register_read_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_motor_register_write_rpc_request_handler_fn)(void *user_data, const motor_register_write_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  motor_register_write_request_t *request_scratch;
  motor_register_write_response_t *response_scratch;
  fci_arm_motor_register_write_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_motor_register_write_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_motor_set_zero_rpc_request_handler_fn)(void *user_data, const motor_set_zero_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  motor_set_zero_request_t *request_scratch;
  motor_set_zero_response_t *response_scratch;
  fci_arm_motor_set_zero_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_motor_set_zero_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_motor_store_parameters_rpc_request_handler_fn)(void *user_data, const motor_store_parameters_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  motor_store_parameters_request_t *request_scratch;
  motor_store_parameters_response_t *response_scratch;
  fci_arm_motor_store_parameters_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_motor_store_parameters_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_release_control_lease_rpc_request_handler_fn)(void *user_data, const release_control_lease_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  release_control_lease_request_t *request_scratch;
  release_control_lease_response_t *response_scratch;
  fci_arm_release_control_lease_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_release_control_lease_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_set_arm_control_mode_rpc_request_handler_fn)(void *user_data, const set_arm_control_mode_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  set_arm_control_mode_request_t *request_scratch;
  set_arm_control_mode_response_t *response_scratch;
  fci_arm_set_arm_control_mode_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_set_arm_control_mode_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_set_arm_mode_rpc_request_handler_fn)(void *user_data, const set_arm_mode_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  set_arm_mode_request_t *request_scratch;
  set_arm_mode_response_t *response_scratch;
  fci_arm_set_arm_mode_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_set_arm_mode_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_set_device_info_rpc_request_handler_fn)(void *user_data, const set_device_info_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  set_device_info_request_t *request_scratch;
  set_device_info_response_t *response_scratch;
  fci_arm_set_device_info_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_set_device_info_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_set_device_settings_rpc_request_handler_fn)(void *user_data, const set_device_settings_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  set_device_settings_request_t *request_scratch;
  set_device_settings_response_t *response_scratch;
  fci_arm_set_device_settings_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_set_device_settings_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_set_gripper_control_mode_rpc_request_handler_fn)(void *user_data, const set_gripper_control_mode_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  set_gripper_control_mode_request_t *request_scratch;
  set_gripper_control_mode_response_t *response_scratch;
  fci_arm_set_gripper_control_mode_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_set_gripper_control_mode_rpc_t;

/* The decoded request and its borrowed fields are valid only for the
 * callback. Copy server_request for asynchronous completion. Its generation
 * prevents a late completion from targeting a reused request identity. A
 * nonzero return abandons this exact pending operation. */
typedef int32_t (*fci_arm_set_zero_rpc_request_handler_fn)(void *user_data, const set_zero_request_t *request, const wl_rpc_server_request_t *server_request, wl_delivery_t delivery);
typedef struct {
  set_zero_request_t *request_scratch;
  set_zero_response_t *response_scratch;
  fci_arm_set_zero_rpc_request_handler_fn request_handler;
  void *user_data;
} fci_arm_set_zero_rpc_t;

/* Shared by synchronous RPC encoders. Runtime APIs are owner-thread
 * operations and do not retain pointers to this scratch after return. */
typedef union {
  acquire_control_lease_request_t acquire_control_lease_request;
  acquire_control_lease_response_t acquire_control_lease_response;
  clear_error_request_t clear_error_request;
  clear_error_response_t clear_error_response;
  clear_faults_request_t clear_faults_request;
  clear_faults_response_t clear_faults_response;
  emergency_stop_request_t emergency_stop_request;
  emergency_stop_response_t emergency_stop_response;
  get_device_info_request_t get_device_info_request;
  get_device_info_response_t get_device_info_response;
  get_device_settings_request_t get_device_settings_request;
  get_device_settings_response_t get_device_settings_response;
  get_motor_feedback_request_t get_motor_feedback_request;
  get_motor_feedback_response_t get_motor_feedback_response;
  home_request_t home_request;
  home_response_t home_response;
  motor_register_read_request_t motor_register_read_request;
  motor_register_read_response_t motor_register_read_response;
  motor_register_write_request_t motor_register_write_request;
  motor_register_write_response_t motor_register_write_response;
  motor_set_zero_request_t motor_set_zero_request;
  motor_set_zero_response_t motor_set_zero_response;
  motor_store_parameters_request_t motor_store_parameters_request;
  motor_store_parameters_response_t motor_store_parameters_response;
  release_control_lease_request_t release_control_lease_request;
  release_control_lease_response_t release_control_lease_response;
  set_arm_control_mode_request_t set_arm_control_mode_request;
  set_arm_control_mode_response_t set_arm_control_mode_response;
  set_arm_mode_request_t set_arm_mode_request;
  set_arm_mode_response_t set_arm_mode_response;
  set_device_info_request_t set_device_info_request;
  set_device_info_response_t set_device_info_response;
  set_device_settings_request_t set_device_settings_request;
  set_device_settings_response_t set_device_settings_response;
  set_gripper_control_mode_request_t set_gripper_control_mode_request;
  set_gripper_control_mode_response_t set_gripper_control_mode_response;
  set_zero_request_t set_zero_request;
  set_zero_response_t set_zero_response;
} fci_arm_runtime_rpc_encode_scratch_t;

typedef struct {
  uint16_t client_timed_out;
  uint16_t server_pending_expired;
  uint16_t server_cache_expired;
  wl_rpc_server_request_t server_expired_request;
} fci_arm_runtime_poll_result_t;

typedef struct {
  fci_arm_runtime_poll_result_t deadlines;
  fci_arm_runtime_result_t response;
  uint16_t responses_submitted;
  uint16_t responses_deferred;
} fci_arm_runtime_service_result_t;

typedef struct {
  uint8_t _reserved;
  wl_latest_t *arm_status_latest;
  wl_latest_t *motor_feedback_latest;
  wl_latest_t *arm_diagnostics_latest;
  wl_rpc_client_t *rpc_client;
  wl_rpc_server_t *rpc_server;
  wl_rpc_peer_t rpc_peer;
  wl_rpc_peer_observation_t rpc_peer_observation;
  fci_arm_runtime_rpc_encode_scratch_t *rpc_encode_scratch;
  fci_arm_acquire_control_lease_rpc_t acquire_control_lease;
  fci_arm_clear_error_rpc_t clear_error;
  fci_arm_clear_faults_rpc_t clear_faults;
  fci_arm_emergency_stop_rpc_t emergency_stop;
  fci_arm_get_device_info_rpc_t get_device_info;
  fci_arm_get_device_settings_rpc_t get_device_settings;
  fci_arm_get_motor_feedback_rpc_t get_motor_feedback;
  fci_arm_home_rpc_t home;
  fci_arm_motor_register_read_rpc_t motor_register_read;
  fci_arm_motor_register_write_rpc_t motor_register_write;
  fci_arm_motor_set_zero_rpc_t motor_set_zero;
  fci_arm_motor_store_parameters_rpc_t motor_store_parameters;
  fci_arm_release_control_lease_rpc_t release_control_lease;
  fci_arm_set_arm_control_mode_rpc_t set_arm_control_mode;
  fci_arm_set_arm_mode_rpc_t set_arm_mode;
  fci_arm_set_device_info_rpc_t set_device_info;
  fci_arm_set_device_settings_rpc_t set_device_settings;
  fci_arm_set_gripper_control_mode_rpc_t set_gripper_control_mode;
  fci_arm_set_zero_rpc_t set_zero;
} fci_arm_runtime_t;

typedef void (*fci_arm_runtime_result_fn)(void *user_data, const fci_arm_runtime_result_t *result);

typedef struct {
  fci_arm_runtime_t *runtime;
  void *user_data;
  fci_arm_runtime_result_fn on_result;
  wl_rpc_err_t last_service_result;
  fci_arm_runtime_service_result_t last_service;
} fci_arm_runtime_pump_t;

/* Static runtime assembly. requirements() validates every sizing field and
 * reports the exact caller-owned byte storage needed by init(). Configuration
 * and storage descriptors may be temporary; instance and storage must outlive
 * all runtime activity and must not be copied after successful initialization. */
typedef struct {
  uint8_t _reserved;
  uint32_t arm_status_latest_initial_generation;
  uint32_t motor_feedback_latest_initial_generation;
  uint32_t arm_diagnostics_latest_initial_generation;
  uint8_t rpc_client_enabled;
  uint16_t rpc_client_slot_count;
  uint16_t rpc_client_response_capacity;
  uint32_t rpc_client_next_operation_id;
  uint8_t rpc_server_enabled;
  uint16_t rpc_server_pending_slot_count;
  uint16_t rpc_server_cache_slot_count;
  uint16_t rpc_server_response_capacity;
  uint32_t rpc_server_pending_timeout_ms;
  uint32_t rpc_server_cache_ttl_ms;
  wl_rpc_cache_policy_t rpc_server_cache_policy;
  fci_arm_acquire_control_lease_rpc_request_handler_fn acquire_control_lease_request_handler;
  void *acquire_control_lease_user_data;
  fci_arm_clear_error_rpc_request_handler_fn clear_error_request_handler;
  void *clear_error_user_data;
  fci_arm_clear_faults_rpc_request_handler_fn clear_faults_request_handler;
  void *clear_faults_user_data;
  fci_arm_emergency_stop_rpc_request_handler_fn emergency_stop_request_handler;
  void *emergency_stop_user_data;
  fci_arm_get_device_info_rpc_request_handler_fn get_device_info_request_handler;
  void *get_device_info_user_data;
  fci_arm_get_device_settings_rpc_request_handler_fn get_device_settings_request_handler;
  void *get_device_settings_user_data;
  fci_arm_get_motor_feedback_rpc_request_handler_fn get_motor_feedback_request_handler;
  void *get_motor_feedback_user_data;
  fci_arm_home_rpc_request_handler_fn home_request_handler;
  void *home_user_data;
  fci_arm_motor_register_read_rpc_request_handler_fn motor_register_read_request_handler;
  void *motor_register_read_user_data;
  fci_arm_motor_register_write_rpc_request_handler_fn motor_register_write_request_handler;
  void *motor_register_write_user_data;
  fci_arm_motor_set_zero_rpc_request_handler_fn motor_set_zero_request_handler;
  void *motor_set_zero_user_data;
  fci_arm_motor_store_parameters_rpc_request_handler_fn motor_store_parameters_request_handler;
  void *motor_store_parameters_user_data;
  fci_arm_release_control_lease_rpc_request_handler_fn release_control_lease_request_handler;
  void *release_control_lease_user_data;
  fci_arm_set_arm_control_mode_rpc_request_handler_fn set_arm_control_mode_request_handler;
  void *set_arm_control_mode_user_data;
  fci_arm_set_arm_mode_rpc_request_handler_fn set_arm_mode_request_handler;
  void *set_arm_mode_user_data;
  fci_arm_set_device_info_rpc_request_handler_fn set_device_info_request_handler;
  void *set_device_info_user_data;
  fci_arm_set_device_settings_rpc_request_handler_fn set_device_settings_request_handler;
  void *set_device_settings_user_data;
  fci_arm_set_gripper_control_mode_rpc_request_handler_fn set_gripper_control_mode_request_handler;
  void *set_gripper_control_mode_user_data;
  fci_arm_set_zero_rpc_request_handler_fn set_zero_request_handler;
  void *set_zero_user_data;
} fci_arm_runtime_config_t;

#define FCI_ARM_RUNTIME_HAS_DEFAULT_STORAGE 1
typedef union {
  uint8_t byte;
  arm_status_t arm_status_latest;
  motor_feedback_t motor_feedback_latest;
  arm_diagnostics_t arm_diagnostics_latest;
  wl_rpc_client_slot_t rpc_client_slot;
  wl_rpc_server_pending_slot_t rpc_server_pending_slot;
  wl_rpc_server_cache_slot_t rpc_server_cache_slot;
} fci_arm_runtime_default_storage_alignment_t;

#if defined(__cplusplus)
#define FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT alignof(fci_arm_runtime_default_storage_alignment_t)
#elif defined(_MSC_VER)
#define FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT __alignof(fci_arm_runtime_default_storage_alignment_t)
#else
#define FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT _Alignof(fci_arm_runtime_default_storage_alignment_t)
#endif
#define FCI_ARM_RUNTIME_DEFAULT_STORAGE_CAPACITY \
  (1U + \
   ((FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U) + ((sizeof(arm_status_t) + (FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U)) * WL_LATEST_SLOT_COUNT)) + \
   ((FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U) + ((sizeof(motor_feedback_t) + (FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U)) * WL_LATEST_SLOT_COUNT)) + \
   ((FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U) + ((sizeof(arm_diagnostics_t) + (FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U)) * WL_LATEST_SLOT_COUNT)) + \
   ((FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U) + sizeof(wl_rpc_client_slot_t)) + \
   218U + \
   ((FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U) + sizeof(wl_rpc_server_pending_slot_t)) + \
   ((FCI_ARM_RUNTIME_DEFAULT_STORAGE_ALIGNMENT - 1U) + sizeof(wl_rpc_server_cache_slot_t)) + \
   218U)

typedef union {
  fci_arm_runtime_default_storage_alignment_t alignment;
  uint8_t bytes[FCI_ARM_RUNTIME_DEFAULT_STORAGE_CAPACITY];
} fci_arm_runtime_default_storage_t;

typedef union { acquire_control_lease_request_t request; acquire_control_lease_response_t response; } fci_arm_runtime_acquire_control_lease_decode_detail_t;
typedef union { clear_error_request_t request; clear_error_response_t response; } fci_arm_runtime_clear_error_decode_detail_t;
typedef union { clear_faults_request_t request; clear_faults_response_t response; } fci_arm_runtime_clear_faults_decode_detail_t;
typedef union { emergency_stop_request_t request; emergency_stop_response_t response; } fci_arm_runtime_emergency_stop_decode_detail_t;
typedef union { get_device_info_request_t request; get_device_info_response_t response; } fci_arm_runtime_get_device_info_decode_detail_t;
typedef union { get_device_settings_request_t request; get_device_settings_response_t response; } fci_arm_runtime_get_device_settings_decode_detail_t;
typedef union { get_motor_feedback_request_t request; get_motor_feedback_response_t response; } fci_arm_runtime_get_motor_feedback_decode_detail_t;
typedef union { home_request_t request; home_response_t response; } fci_arm_runtime_home_decode_detail_t;
typedef union { motor_register_read_request_t request; motor_register_read_response_t response; } fci_arm_runtime_motor_register_read_decode_detail_t;
typedef union { motor_register_write_request_t request; motor_register_write_response_t response; } fci_arm_runtime_motor_register_write_decode_detail_t;
typedef union { motor_set_zero_request_t request; motor_set_zero_response_t response; } fci_arm_runtime_motor_set_zero_decode_detail_t;
typedef union { motor_store_parameters_request_t request; motor_store_parameters_response_t response; } fci_arm_runtime_motor_store_parameters_decode_detail_t;
typedef union { release_control_lease_request_t request; release_control_lease_response_t response; } fci_arm_runtime_release_control_lease_decode_detail_t;
typedef union { set_arm_control_mode_request_t request; set_arm_control_mode_response_t response; } fci_arm_runtime_set_arm_control_mode_decode_detail_t;
typedef union { set_arm_mode_request_t request; set_arm_mode_response_t response; } fci_arm_runtime_set_arm_mode_decode_detail_t;
typedef union { set_device_info_request_t request; set_device_info_response_t response; } fci_arm_runtime_set_device_info_decode_detail_t;
typedef union { set_device_settings_request_t request; set_device_settings_response_t response; } fci_arm_runtime_set_device_settings_decode_detail_t;
typedef union { set_gripper_control_mode_request_t request; set_gripper_control_mode_response_t response; } fci_arm_runtime_set_gripper_control_mode_decode_detail_t;
typedef union { set_zero_request_t request; set_zero_response_t response; } fci_arm_runtime_set_zero_decode_detail_t;
typedef struct {
  size_t storage_size;
  size_t storage_alignment;
} fci_arm_runtime_requirements_t;

typedef struct {
  void *data;
  size_t size;
} fci_arm_runtime_storage_t;

typedef struct {
  fci_arm_runtime_t runtime;
  wl_latest_t arm_status_latest;
  wl_latest_t motor_feedback_latest;
  wl_latest_t arm_diagnostics_latest;
  wl_rpc_client_t rpc_client;
  wl_rpc_server_t rpc_server;
  fci_arm_runtime_rpc_encode_scratch_t rpc_encode_scratch;
  /* One dispatch at a time: bounded services share decode scratch.
   * Views are callback-scoped; deferred work must copy its input. */
  union {
    fci_arm_runtime_acquire_control_lease_decode_detail_t acquire_control_lease_scratch;
    fci_arm_runtime_clear_error_decode_detail_t clear_error_scratch;
    fci_arm_runtime_clear_faults_decode_detail_t clear_faults_scratch;
    fci_arm_runtime_emergency_stop_decode_detail_t emergency_stop_scratch;
    fci_arm_runtime_get_device_info_decode_detail_t get_device_info_scratch;
    fci_arm_runtime_get_device_settings_decode_detail_t get_device_settings_scratch;
    fci_arm_runtime_get_motor_feedback_decode_detail_t get_motor_feedback_scratch;
    fci_arm_runtime_home_decode_detail_t home_scratch;
    fci_arm_runtime_motor_register_read_decode_detail_t motor_register_read_scratch;
    fci_arm_runtime_motor_register_write_decode_detail_t motor_register_write_scratch;
    fci_arm_runtime_motor_set_zero_decode_detail_t motor_set_zero_scratch;
    fci_arm_runtime_motor_store_parameters_decode_detail_t motor_store_parameters_scratch;
    fci_arm_runtime_release_control_lease_decode_detail_t release_control_lease_scratch;
    fci_arm_runtime_set_arm_control_mode_decode_detail_t set_arm_control_mode_scratch;
    fci_arm_runtime_set_arm_mode_decode_detail_t set_arm_mode_scratch;
    fci_arm_runtime_set_device_info_decode_detail_t set_device_info_scratch;
    fci_arm_runtime_set_device_settings_decode_detail_t set_device_settings_scratch;
    fci_arm_runtime_set_gripper_control_mode_decode_detail_t set_gripper_control_mode_scratch;
    fci_arm_runtime_set_zero_decode_detail_t set_zero_scratch;
  };
} fci_arm_runtime_instance_t;

typedef int32_t fci_arm_runtime_init_issue_t;
enum {
  FCI_ARM_RUNTIME_INIT_OK = 0,
  FCI_ARM_RUNTIME_INIT_NULL_ARGUMENT,
  FCI_ARM_RUNTIME_INIT_ROLE_ENABLE,
  FCI_ARM_RUNTIME_INIT_RETAINED_CAPACITY,
  FCI_ARM_RUNTIME_INIT_RPC_CLIENT_CAPACITY,
  FCI_ARM_RUNTIME_INIT_RPC_SERVER_CAPACITY,
  FCI_ARM_RUNTIME_INIT_RPC_TIMEOUT,
  FCI_ARM_RUNTIME_INIT_RPC_CACHE_POLICY,
  FCI_ARM_RUNTIME_INIT_LAYOUT_OVERFLOW,
  FCI_ARM_RUNTIME_INIT_STORAGE_TOO_SMALL,
  FCI_ARM_RUNTIME_INIT_STORAGE_NULL,
  FCI_ARM_RUNTIME_INIT_STORAGE_ALIGNMENT,
  FCI_ARM_RUNTIME_INIT_STORAGE_OVERLAP,
  FCI_ARM_RUNTIME_INIT_COMPONENT
};

typedef struct {
  fci_arm_runtime_init_issue_t issue;
  const char *field;
  size_t required;
  size_t provided;
} fci_arm_runtime_init_diagnostic_t;

const char *fci_arm_runtime_init_issue_str(fci_arm_runtime_init_issue_t issue);

/* Mechanical defaults use one FIFO/RPC slot, generation/operation ID one,
 * bounded encoded maxima, disabled roles, zero timeouts, and reject-new cache.
 * Override policy fields after this call. */
wl_err_t fci_arm_runtime_config_defaults(fci_arm_runtime_config_t *config);

wl_err_t fci_arm_runtime_config_enable_client(fci_arm_runtime_config_t *config);
wl_err_t fci_arm_runtime_config_enable_server(fci_arm_runtime_config_t *config);
fci_arm_runtime_storage_t fci_arm_runtime_default_storage_descriptor(fci_arm_runtime_default_storage_t *storage);
int fci_arm_runtime_requirements(const fci_arm_runtime_config_t *config, fci_arm_runtime_requirements_t *out_requirements);
/* Checked initialization reports the exact rejected field and capacity values. */
int fci_arm_runtime_init_checked(fci_arm_runtime_instance_t *instance, const fci_arm_runtime_config_t *config, const fci_arm_runtime_storage_t *storage, fci_arm_runtime_init_diagnostic_t *out_diagnostic);
int fci_arm_runtime_init(fci_arm_runtime_instance_t *instance, const fci_arm_runtime_config_t *config, const fci_arm_runtime_storage_t *storage);
/* With non-null ctx/event every RX outcome is consumed. Matching RPC TX
 * terminal events advance the runtime and reclaim the handle. Inspect
 * result.event_consumed before applying a fallback owner action. */
fci_arm_runtime_result_t fci_arm_runtime_dispatch_event(wl_ctx_t *ctx, const wl_event_t *event, fci_arm_runtime_t *runtime, wl_time_ms_t now_ms);

int fci_arm_arm_status_latest_acquire(fci_arm_runtime_t *runtime, fci_arm_arm_status_latest_view_t *out_view);
int fci_arm_arm_status_latest_release(fci_arm_runtime_t *runtime, fci_arm_arm_status_latest_view_t *view);

int fci_arm_motor_feedback_latest_acquire(fci_arm_runtime_t *runtime, fci_arm_motor_feedback_latest_view_t *out_view);
int fci_arm_motor_feedback_latest_release(fci_arm_runtime_t *runtime, fci_arm_motor_feedback_latest_view_t *view);

int fci_arm_arm_diagnostics_latest_acquire(fci_arm_runtime_t *runtime, fci_arm_arm_diagnostics_latest_view_t *out_view);
int fci_arm_arm_diagnostics_latest_release(fci_arm_runtime_t *runtime, fci_arm_arm_diagnostics_latest_view_t *view);

/* Observe a nonzero point-to-point peer before non-RPC traffic is handled. */
wl_rpc_err_t fci_arm_runtime_peer_observe(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, uint64_t peer_session_id, wl_rpc_peer_observation_t *out_observation);
/* A reliable server request automatically observes its peer session before
 * dispatch. Take a changed observation to revoke product leases/non-RPC work. */
wl_rpc_err_t fci_arm_runtime_peer_observation_take(fci_arm_runtime_t *runtime, wl_rpc_peer_observation_t *out_observation);
/* Advance configured RPC deadlines without performing I/O. At most one
 * expired server identity is returned per call and remains pending until the
 * application completes, rejects, or abandons it. */
wl_rpc_err_t fci_arm_runtime_poll(fci_arm_runtime_t *runtime, wl_time_ms_t now_ms, fci_arm_runtime_poll_result_t *out_result);
/* Advance deadlines and submit at most one runtime-owned server response.
 * Link backpressure defers the same cached bytes for a later service call. */
wl_rpc_err_t fci_arm_runtime_service(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, wl_time_ms_t now_ms, fci_arm_runtime_service_result_t *out_result);
/* Side-effect free. Zero is due; WL_RPC_NO_DEADLINE_MS means no deadline. */
wl_rpc_err_t fci_arm_runtime_get_deadline_hint(const fci_arm_runtime_t *runtime, wl_time_ms_t now_ms, wl_rpc_deadline_hint_t *out_hint);

/* Build pump hooks that dispatch events with the owner's time sample. RPC
 * profiles also service one queued response per pass and merge their deadline.
 * on_result may be null; result pointers are borrowed only for the callback. */
wl_err_t fci_arm_runtime_pump_init(fci_arm_runtime_pump_t *pump, fci_arm_runtime_t *runtime, fci_arm_runtime_result_fn on_result, void *user_data);
wl_pump_hooks_t fci_arm_runtime_pump_hooks(fci_arm_runtime_pump_t *pump);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const acquire_control_lease_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_acquire_control_lease_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_decode(const wl_rpc_client_result_t *client, acquire_control_lease_response_t *response);
wl_rpc_err_t fci_arm_acquire_control_lease_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const acquire_control_lease_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const acquire_control_lease_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_clear_error_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const clear_error_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_clear_error_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_clear_error_client_decode(const wl_rpc_client_result_t *client, clear_error_response_t *response);
wl_rpc_err_t fci_arm_clear_error_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_clear_error_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const clear_error_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_clear_error_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const clear_error_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_clear_faults_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const clear_faults_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_clear_faults_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_clear_faults_client_decode(const wl_rpc_client_result_t *client, clear_faults_response_t *response);
wl_rpc_err_t fci_arm_clear_faults_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_clear_faults_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const clear_faults_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_clear_faults_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const clear_faults_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_emergency_stop_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const emergency_stop_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_emergency_stop_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_emergency_stop_client_decode(const wl_rpc_client_result_t *client, emergency_stop_response_t *response);
wl_rpc_err_t fci_arm_emergency_stop_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_emergency_stop_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const emergency_stop_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_emergency_stop_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const emergency_stop_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_get_device_info_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const get_device_info_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_get_device_info_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_get_device_info_client_decode(const wl_rpc_client_result_t *client, get_device_info_response_t *response);
wl_rpc_err_t fci_arm_get_device_info_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_get_device_info_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const get_device_info_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_get_device_info_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_device_info_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_get_device_settings_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const get_device_settings_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_get_device_settings_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_get_device_settings_client_decode(const wl_rpc_client_result_t *client, get_device_settings_response_t *response);
wl_rpc_err_t fci_arm_get_device_settings_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_get_device_settings_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const get_device_settings_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_get_device_settings_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_device_settings_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const get_motor_feedback_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_get_motor_feedback_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_decode(const wl_rpc_client_result_t *client, get_motor_feedback_response_t *response);
wl_rpc_err_t fci_arm_get_motor_feedback_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const get_motor_feedback_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_motor_feedback_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_home_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const home_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_home_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_home_client_decode(const wl_rpc_client_result_t *client, home_response_t *response);
wl_rpc_err_t fci_arm_home_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_home_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const home_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_home_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const home_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_motor_register_read_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_register_read_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_motor_register_read_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_motor_register_read_client_decode(const wl_rpc_client_result_t *client, motor_register_read_response_t *response);
wl_rpc_err_t fci_arm_motor_register_read_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_motor_register_read_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_register_read_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_motor_register_read_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_register_read_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_motor_register_write_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_register_write_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_motor_register_write_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_motor_register_write_client_decode(const wl_rpc_client_result_t *client, motor_register_write_response_t *response);
wl_rpc_err_t fci_arm_motor_register_write_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_motor_register_write_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_register_write_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_motor_register_write_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_register_write_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_motor_set_zero_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_set_zero_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_motor_set_zero_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_motor_set_zero_client_decode(const wl_rpc_client_result_t *client, motor_set_zero_response_t *response);
wl_rpc_err_t fci_arm_motor_set_zero_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_motor_set_zero_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_set_zero_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_motor_set_zero_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_set_zero_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_store_parameters_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_motor_store_parameters_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_decode(const wl_rpc_client_result_t *client, motor_store_parameters_response_t *response);
wl_rpc_err_t fci_arm_motor_store_parameters_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_store_parameters_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_store_parameters_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_release_control_lease_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const release_control_lease_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_release_control_lease_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_release_control_lease_client_decode(const wl_rpc_client_result_t *client, release_control_lease_response_t *response);
wl_rpc_err_t fci_arm_release_control_lease_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_release_control_lease_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const release_control_lease_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_release_control_lease_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const release_control_lease_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_arm_control_mode_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_set_arm_control_mode_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_decode(const wl_rpc_client_result_t *client, set_arm_control_mode_response_t *response);
wl_rpc_err_t fci_arm_set_arm_control_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_arm_control_mode_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_arm_control_mode_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_set_arm_mode_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_arm_mode_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_set_arm_mode_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_set_arm_mode_client_decode(const wl_rpc_client_result_t *client, set_arm_mode_response_t *response);
wl_rpc_err_t fci_arm_set_arm_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_set_arm_mode_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_arm_mode_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_set_arm_mode_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_arm_mode_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_set_device_info_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_device_info_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_set_device_info_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_set_device_info_client_decode(const wl_rpc_client_result_t *client, set_device_info_response_t *response);
wl_rpc_err_t fci_arm_set_device_info_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_set_device_info_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_device_info_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_set_device_info_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_device_info_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_set_device_settings_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_device_settings_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_set_device_settings_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_set_device_settings_client_decode(const wl_rpc_client_result_t *client, set_device_settings_response_t *response);
wl_rpc_err_t fci_arm_set_device_settings_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_set_device_settings_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_device_settings_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_set_device_settings_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_device_settings_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_gripper_control_mode_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_set_gripper_control_mode_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_decode(const wl_rpc_client_result_t *client, set_gripper_control_mode_response_t *response);
wl_rpc_err_t fci_arm_set_gripper_control_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_gripper_control_mode_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_gripper_control_mode_response_t *response, wl_time_ms_t now_ms);

/* Allocates, encodes, and submits atomically from the caller's view. A
 * present nonzero request operation ID is used exactly, allowing an explicit
 * retry to address the server's bounded replay cache; absent or zero selects an
 * automatically allocated ID. The const request is copied into runtime-owned
 * encode scratch before operation ID injection. A local encode/submit failure
 * releases the allocated RPC slot and returns operation_id zero. */
fci_arm_runtime_result_t fci_arm_set_zero_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_zero_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms);
/* Nonblocking inspection returns generic metadata for this service. */
wl_rpc_err_t fci_arm_set_zero_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Decode a retained response previously returned by client_inspect(). Borrowed
 * response fields remain valid only until client_release(). */
fci_arm_runtime_result_t fci_arm_set_zero_client_decode(const wl_rpc_client_result_t *client, set_zero_response_t *response);
wl_rpc_err_t fci_arm_set_zero_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);

/* server_request is copied from the request callback and uniquely scopes this
 * execution generation. Completion copies the const response into runtime-owned
 * encode scratch before injecting operation ID/status. runtime_service() later
 * submits the cached response bytes. */
fci_arm_runtime_result_t fci_arm_set_zero_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_zero_response_t *response, wl_time_ms_t now_ms);
fci_arm_runtime_result_t fci_arm_set_zero_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_zero_response_t *response, wl_time_ms_t now_ms);

/* Default endpoint assembly. Members named private_state are not application
 * API. Zero-initialize once, keep at a stable address, close before reuse.
 * All profile-selected messages must have finite one-frame bounds. */
#define FCI_ARM_HAS_DEFAULT_ENDPOINT 1
#define FCI_ARM_ENDPOINT_MAX_PAYLOAD 233U
/* Layout is fixed by the local profile; CRC32C bounds also cover smaller CRCs. */
#define FCI_ARM_ENDPOINT_RAW_CAPACITY (FCI_ARM_ENDPOINT_MAX_PAYLOAD + WL_FRAME_HEADER_SIZE + WL_FRAME_MAX_CRC)
#define FCI_ARM_ENDPOINT_UNIT_CAPACITY (FCI_ARM_ENDPOINT_RAW_CAPACITY + FCI_ARM_ENDPOINT_RAW_CAPACITY / 254U + 2U)
#define FCI_ARM_ENDPOINT_CONTROL_CAPACITY (WL_FRAME_HEADER_SIZE + WL_FRAME_MAX_CRC + 2U)
#define FCI_ARM_ENDPOINT_RX_FIFO_CAPACITY FCI_ARM_ENDPOINT_UNIT_CAPACITY
#define FCI_ARM_ENDPOINT_RUNTIME_CAPACITY FCI_ARM_RUNTIME_DEFAULT_STORAGE_CAPACITY

typedef struct {
  wl_config_t link;
  wl_environment_t environment;
  /* Expert policy/storage overrides; ordinary applications use defaults. */
  fci_arm_runtime_config_t advanced;

  size_t event_budget;
  fci_arm_runtime_result_fn on_result;
  /* Shared context for on_result and ordinary on_<rpc> handlers. Advanced
   * deferred handlers and per-call completion contexts remain explicit. */
  void *user_data;
} fci_arm_endpoint_config_t;

typedef struct {
  struct {
    wl_endpoint_t owner;
    wl_allocator_t allocator; /* Zero for caller-owned static storage. */
    bool stepping;
    bool closing;
    uint64_t previous_session;
    fci_arm_runtime_instance_t instance;
    union {
      fci_arm_runtime_default_storage_alignment_t alignment;
      uint8_t bytes[FCI_ARM_ENDPOINT_RUNTIME_CAPACITY];
    } arena;
    fci_arm_runtime_pump_t pump;
    fci_arm_runtime_result_t result;
    fci_arm_runtime_result_fn on_result;
    void *user_data;
    size_t event_budget;

    uint8_t tx_payload[FCI_ARM_ENDPOINT_MAX_PAYLOAD];
    uint8_t tx_unit[FCI_ARM_ENDPOINT_UNIT_CAPACITY];
    uint8_t control_unit[FCI_ARM_ENDPOINT_CONTROL_CAPACITY];
    uint8_t rx_fallback[FCI_ARM_ENDPOINT_UNIT_CAPACITY];
    uint8_t rx_fifo[FCI_ARM_ENDPOINT_RX_FIFO_CAPACITY];
  } private_state;
} fci_arm_endpoint_t;

#if defined(__cplusplus)
#define FCI_ARM_ENDPOINT_ALIGNMENT alignof(fci_arm_endpoint_t)
#elif defined(_MSC_VER)
#define FCI_ARM_ENDPOINT_ALIGNMENT __alignof(fci_arm_endpoint_t)
#else
#define FCI_ARM_ENDPOINT_ALIGNMENT _Alignof(fci_arm_endpoint_t)
#endif

/* Config descriptors can be temporary; callbacks/user_data must outlive use.
 * Ordinary client capability and registered server handlers are assembled by init. */
static inline wl_err_t fci_arm_endpoint_config_defaults(
    fci_arm_endpoint_config_t *config, wl_environment_t environment) {
  int result;
  if (config == NULL) return WL_ERR_INVALID_ARG;
  memset(config, 0, sizeof(*config));
  config->link.max_payload_len = FCI_ARM_ENDPOINT_MAX_PAYLOAD;
  config->link.envelope = WL_ENVELOPE_NATIVE_PACKET;
  config->link.integrity = WL_INTEGRITY_CRC32C;
  config->environment = environment;
  config->link.ack_timeout_ms = 100U;
  config->link.max_retries = 4U;
  config->event_budget = 16U;
  result = fci_arm_runtime_config_defaults(&config->advanced);
  if (result != WL_OK) return result;

  return WL_OK;
}

static inline wl_endpoint_t *fci_arm_endpoint_handle(fci_arm_endpoint_t *endpoint) {
  return endpoint != NULL ? &endpoint->private_state.owner : NULL;
}

/* Advanced integration only: do not also dispatch/take events consumed by step. */
static inline fci_arm_runtime_t *fci_arm_endpoint_runtime(fci_arm_endpoint_t *endpoint) {
  return endpoint != NULL && wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) != NULL
      ? &endpoint->private_state.instance.runtime : NULL;
}

static inline void fci_arm_endpoint_record(void *context,
                                      const fci_arm_runtime_result_t *result) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  fci_arm_runtime_result_t terminal;
  if (result->domain == FCI_ARM_RUNTIME_NON_RX) {
    if (result->event_type != WL_EVT_TX_TIMEOUT && result->event_type != WL_EVT_TX_FAILED)
      return; /* Normal transport completion is not an application failure. */
    terminal = *result;
    terminal.domain = FCI_ARM_RUNTIME_CORE_ERROR;
    result = &terminal;
  }
#if FCI_ARM_RUNTIME_HAS_MANAGED_RPC
  /* Cached replies retain their reservation and retry ordinary link pressure.
   * Per-call failure/deadline remains observable through completion. */
  if (result->domain == FCI_ARM_RUNTIME_CORE_ERROR && result->detail_kind == FCI_ARM_RUNTIME_DETAIL_RPC &&
      (result->detail.rpc.core_result == WL_ERR_BUSY ||
       result->detail.rpc.core_result == WL_ERR_WOULD_BLOCK ||
       result->detail.rpc.core_result == WL_ERR_QUEUE_FULL ||
       result->detail.rpc.core_result == WL_ERR_NO_SPACE)) return;
#endif
  /* Retain the first failure even if later events in the same pass succeed. */
  if (fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (endpoint->private_state.on_result != NULL)
    endpoint->private_state.on_result(endpoint->private_state.user_data, result);
}

static inline wl_err_t fci_arm_endpoint_init_config(
    fci_arm_endpoint_t *endpoint, const fci_arm_endpoint_config_t *config) {
  wl_storage_t link_storage;
  fci_arm_runtime_storage_t storage;
  wl_pump_hooks_t hooks;
  fci_arm_runtime_config_t runtime_config;
  wl_config_t link_config;
  int result;
  if (endpoint == NULL || config == NULL || config->event_budget == 0U)
    return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.stepping || endpoint->private_state.closing)
    return WL_ERR_REENTRANT;
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) != NULL)
    return WL_ERR_INVALID_STATE;
  if (config->environment.clock.now_ms == NULL || config->link.session_id != 0U)
    return WL_ERR_INVALID_ARG;

  runtime_config = config->advanced;

  link_config = config->link;
  result = wl_session_next(config->environment.session,
      endpoint->private_state.previous_session, &link_config.session_id);
  if (result != WL_OK) return result;

  memset(&link_storage, 0, sizeof(link_storage));
  link_storage.tx_payload = endpoint->private_state.tx_payload;
  link_storage.tx_payload_size = sizeof(endpoint->private_state.tx_payload);
  link_storage.tx_unit = endpoint->private_state.tx_unit;
  link_storage.tx_unit_size = sizeof(endpoint->private_state.tx_unit);
  link_storage.control_unit = endpoint->private_state.control_unit;
  link_storage.control_unit_size = sizeof(endpoint->private_state.control_unit);
  link_storage.rx_fallback = endpoint->private_state.rx_fallback;
  link_storage.rx_fallback_size = sizeof(endpoint->private_state.rx_fallback);
  link_storage.rx_fifo = endpoint->private_state.rx_fifo;
  link_storage.rx_fifo_size = sizeof(endpoint->private_state.rx_fifo);
  storage.data = endpoint->private_state.arena.bytes;
  storage.size = sizeof(endpoint->private_state.arena.bytes);
  result = fci_arm_runtime_init(&endpoint->private_state.instance, &runtime_config, &storage);
  if (result != WL_OK) return result;


  result = fci_arm_runtime_pump_init(&endpoint->private_state.pump,
      &endpoint->private_state.instance.runtime, fci_arm_endpoint_record, endpoint);
  if (result != WL_OK) return result;
  hooks = fci_arm_runtime_pump_hooks(&endpoint->private_state.pump);
  result = wl_endpoint_init(&endpoint->private_state.owner, &link_config,
                            &link_storage, &config->environment.clock, &hooks);
  if (result != WL_OK) return result;
  endpoint->private_state.previous_session = link_config.session_id;

  endpoint->private_state.on_result = config->on_result;
  endpoint->private_state.user_data = config->user_data;
  endpoint->private_state.event_budget = config->event_budget;
  memset(&endpoint->private_state.result, 0, sizeof(endpoint->private_state.result));
  return WL_OK;
}

static inline wl_err_t fci_arm_endpoint_init(fci_arm_endpoint_t *endpoint,
                                        wl_environment_t environment) {
  fci_arm_endpoint_config_t config;
  int result = fci_arm_endpoint_config_defaults(&config, environment);
  return result == WL_OK ? fci_arm_endpoint_init_config(endpoint, &config) : result;
}

/* One bounded owner pass: service transport, dispatch and release events,
 * advance RPC work. NO_DATA/backpressure during transport service is normal.
 * Inspect endpoint_result/last_step for details when this returns an error. */
static inline wl_err_t fci_arm_endpoint_step(fci_arm_endpoint_t *endpoint) {
  int result;
  if (endpoint == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.stepping || endpoint->private_state.closing) return WL_ERR_REENTRANT;
  endpoint->private_state.stepping = true;
  memset(&endpoint->private_state.result, 0, sizeof(endpoint->private_state.result));
  result = wl_endpoint_step(&endpoint->private_state.owner,
                             endpoint->private_state.event_budget);
  endpoint->private_state.stepping = false;
  if (result != WL_OK) return result;
  if (endpoint->private_state.pump.last_service_result != WL_RPC_OK) {
    if (fci_arm_runtime_result_ok(&endpoint->private_state.result)) {
      endpoint->private_state.result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
      endpoint->private_state.result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      endpoint->private_state.result.detail.rpc.rpc_result = endpoint->private_state.pump.last_service_result;
    }
    return WL_ERR_INVALID_STATE;
  }
  return fci_arm_runtime_result_ok(&endpoint->private_state.result)
      ? WL_OK : WL_ERR_INVALID_STATE;
}

static inline const fci_arm_runtime_result_t *fci_arm_endpoint_result(const fci_arm_endpoint_t *endpoint) {
  return endpoint != NULL ? &endpoint->private_state.result : NULL;
}

static inline wl_err_t fci_arm_endpoint_close(fci_arm_endpoint_t *endpoint) {
  if (endpoint == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.stepping || endpoint->private_state.closing) return WL_ERR_REENTRANT;
  endpoint->private_state.closing = true;
  wl_endpoint_close(fci_arm_endpoint_handle(endpoint));

  endpoint->private_state.closing = false;
  return WL_OK;
}

static inline wl_err_t fci_arm_endpoint_driver_step(void *context) {
  return fci_arm_endpoint_step((fci_arm_endpoint_t *)context);
}
static inline wl_err_t fci_arm_endpoint_driver_close(void *context) {
  return fci_arm_endpoint_close((fci_arm_endpoint_t *)context);
}
/* Setup-only platform integration; ordinary code does not drive both objects. */
static inline wl_endpoint_driver_t fci_arm_endpoint_driver(fci_arm_endpoint_t *endpoint) {
  wl_endpoint_driver_t driver;
  driver.endpoint = fci_arm_endpoint_handle(endpoint);
  driver.context = endpoint;
  driver.step = fci_arm_endpoint_driver_step;
  driver.close = fci_arm_endpoint_driver_close;
  return driver;
}

/* Optional creation: one allocation for the complete endpoint and all protocol
 * storage. *out must be NULL; failure leaves it unchanged. No global allocator
 * or fallback heap. Configure transport/waiting after successful creation. */
static inline wl_err_t fci_arm_endpoint_create(fci_arm_endpoint_t **out,
    const fci_arm_endpoint_config_t *config, const wl_allocator_t *allocator) {
  fci_arm_endpoint_t *endpoint;
  wl_allocator_t storage;
  int error;
  if (out == NULL || *out != NULL || config == NULL || allocator == NULL ||
      allocator->allocate == NULL || allocator->deallocate == NULL) return WL_ERR_INVALID_ARG;
  storage = *allocator;
  endpoint = (fci_arm_endpoint_t *)storage.allocate(storage.context,
      sizeof(*endpoint), FCI_ARM_ENDPOINT_ALIGNMENT);
  if (endpoint == NULL) return WL_ERR_NO_MEM;
  if ((uintptr_t)endpoint % FCI_ARM_ENDPOINT_ALIGNMENT != 0U) {
    storage.deallocate(storage.context, endpoint, sizeof(*endpoint), FCI_ARM_ENDPOINT_ALIGNMENT);
    return WL_ERR_INVALID_ARG;
  }
  memset(endpoint, 0, sizeof(*endpoint));
  error = fci_arm_endpoint_init_config(endpoint, config);
  if (error != WL_OK) {
    (void)fci_arm_endpoint_close(endpoint);
    storage.deallocate(storage.context, endpoint, sizeof(*endpoint), FCI_ARM_ENDPOINT_ALIGNMENT);
    return error;
  }
  endpoint->private_state.allocator = storage;
  *out = endpoint;
  return WL_OK;
}

/* Owner safe point, after background executor stop and all caller joins.
 * Close/quiesce and finish callbacks before freeing. Reentrant failure leaves
 * the pointer alive. Static storage must use close, not destroy. NULL is a no-op.
 * Copies of this owning pointer are invalid after successful destruction. */
static inline wl_err_t fci_arm_endpoint_destroy(fci_arm_endpoint_t **owner) {
  fci_arm_endpoint_t *endpoint;
  wl_allocator_t storage;
  int error;
  if (owner == NULL) return WL_ERR_INVALID_ARG;
  endpoint = *owner;
  if (endpoint == NULL) return WL_OK;
  storage = endpoint->private_state.allocator;
  if (storage.deallocate == NULL) return WL_ERR_INVALID_STATE;
  error = fci_arm_endpoint_close(endpoint);
  if (error != WL_OK) return error;
  *owner = NULL;
  storage.deallocate(storage.context, endpoint, sizeof(*endpoint), FCI_ARM_ENDPOINT_ALIGNMENT);
  return WL_OK;
}
/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_arm_status(fci_arm_endpoint_t *endpoint, const arm_status_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_arm_status_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Copy an owned value and release its lease internally. NO_DATA leaves out unchanged. */
static inline wl_err_t fci_arm_endpoint_read_arm_status(fci_arm_endpoint_t *endpoint, arm_status_t *out) {
  fci_arm_arm_status_latest_view_t view;
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  int result;
  if (out == NULL) return WL_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_ERR_NOT_INITIALIZED;
  result = fci_arm_arm_status_latest_acquire(runtime, &view);
  if (result != WL_OK) return result;
  *out = *view.value;
  return fci_arm_arm_status_latest_release(runtime, &view);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_motor_feedback(fci_arm_endpoint_t *endpoint, const motor_feedback_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_motor_feedback_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Copy an owned value and release its lease internally. NO_DATA leaves out unchanged. */
static inline wl_err_t fci_arm_endpoint_read_motor_feedback(fci_arm_endpoint_t *endpoint, motor_feedback_t *out) {
  fci_arm_motor_feedback_latest_view_t view;
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  int result;
  if (out == NULL) return WL_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_ERR_NOT_INITIALIZED;
  result = fci_arm_motor_feedback_latest_acquire(runtime, &view);
  if (result != WL_OK) return result;
  *out = *view.value;
  return fci_arm_motor_feedback_latest_release(runtime, &view);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_arm_diagnostics(fci_arm_endpoint_t *endpoint, const arm_diagnostics_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_arm_diagnostics_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Copy an owned value and release its lease internally. NO_DATA leaves out unchanged. */
static inline wl_err_t fci_arm_endpoint_read_arm_diagnostics(fci_arm_endpoint_t *endpoint, arm_diagnostics_t *out) {
  fci_arm_arm_diagnostics_latest_view_t view;
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  int result;
  if (out == NULL) return WL_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_ERR_NOT_INITIALIZED;
  result = fci_arm_arm_diagnostics_latest_acquire(runtime, &view);
  if (result != WL_OK) return result;
  *out = *view.value;
  return fci_arm_arm_diagnostics_latest_release(runtime, &view);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_acquire_control_lease_start(fci_arm_endpoint_t *endpoint, const acquire_control_lease_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_acquire_control_lease_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_acquire_control_lease_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_acquire_control_lease_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_acquire_control_lease_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const acquire_control_lease_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_acquire_control_lease_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_clear_error_start(fci_arm_endpoint_t *endpoint, const clear_error_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_clear_error_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_error_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_clear_error_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_error_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_clear_error_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_clear_error_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const clear_error_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_clear_error_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_clear_faults_start(fci_arm_endpoint_t *endpoint, const clear_faults_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_clear_faults_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_clear_faults_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_clear_faults_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_clear_faults_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const clear_faults_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_clear_faults_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_emergency_stop_start(fci_arm_endpoint_t *endpoint, const emergency_stop_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_emergency_stop_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_emergency_stop_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_emergency_stop_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_emergency_stop_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const emergency_stop_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_emergency_stop_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_get_device_info_start(fci_arm_endpoint_t *endpoint, const get_device_info_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_get_device_info_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_get_device_info_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_get_device_info_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_get_device_info_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const get_device_info_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_get_device_info_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_get_device_settings_start(fci_arm_endpoint_t *endpoint, const get_device_settings_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_get_device_settings_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_get_device_settings_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_get_device_settings_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_get_device_settings_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const get_device_settings_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_get_device_settings_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_get_motor_feedback_start(fci_arm_endpoint_t *endpoint, const get_motor_feedback_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_get_motor_feedback_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_get_motor_feedback_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_get_motor_feedback_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_get_motor_feedback_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const get_motor_feedback_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_get_motor_feedback_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_home_start(fci_arm_endpoint_t *endpoint, const home_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_home_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_home_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_home_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_home_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_home_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_home_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const home_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_home_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_register_read_start(fci_arm_endpoint_t *endpoint, const motor_register_read_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_register_read_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_motor_register_read_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_motor_register_read_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_register_read_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const motor_register_read_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_register_read_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_register_write_start(fci_arm_endpoint_t *endpoint, const motor_register_write_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_register_write_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_motor_register_write_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_motor_register_write_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_register_write_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const motor_register_write_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_register_write_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_set_zero_start(fci_arm_endpoint_t *endpoint, const motor_set_zero_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_set_zero_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_motor_set_zero_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_motor_set_zero_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_set_zero_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const motor_set_zero_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_set_zero_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_store_parameters_start(fci_arm_endpoint_t *endpoint, const motor_store_parameters_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_store_parameters_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_motor_store_parameters_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_motor_store_parameters_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_motor_store_parameters_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const motor_store_parameters_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_motor_store_parameters_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_release_control_lease_start(fci_arm_endpoint_t *endpoint, const release_control_lease_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_release_control_lease_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_release_control_lease_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_release_control_lease_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_release_control_lease_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const release_control_lease_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_release_control_lease_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_arm_control_mode_start(fci_arm_endpoint_t *endpoint, const set_arm_control_mode_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_arm_control_mode_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_set_arm_control_mode_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_set_arm_control_mode_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_arm_control_mode_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const set_arm_control_mode_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_arm_control_mode_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_arm_mode_start(fci_arm_endpoint_t *endpoint, const set_arm_mode_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_arm_mode_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_set_arm_mode_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_set_arm_mode_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_arm_mode_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const set_arm_mode_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_arm_mode_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_device_info_start(fci_arm_endpoint_t *endpoint, const set_device_info_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_device_info_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_set_device_info_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_set_device_info_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_device_info_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const set_device_info_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_device_info_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_device_settings_start(fci_arm_endpoint_t *endpoint, const set_device_settings_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_device_settings_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_set_device_settings_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_set_device_settings_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_device_settings_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const set_device_settings_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_device_settings_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_gripper_control_mode_start(fci_arm_endpoint_t *endpoint, const set_gripper_control_mode_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_gripper_control_mode_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_set_gripper_control_mode_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_set_gripper_control_mode_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_gripper_control_mode_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const set_gripper_control_mode_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_gripper_control_mode_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_zero_start(fci_arm_endpoint_t *endpoint, const set_zero_request_t *request, uint32_t timeout_ms) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_zero_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), fci_arm_endpoint_runtime(endpoint), request, timeout_ms, now_ms);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_zero_inspect(fci_arm_endpoint_t *endpoint, uint32_t operation_id, wl_rpc_client_result_t *result) {
  return fci_arm_set_zero_client_inspect(fci_arm_endpoint_runtime(endpoint), operation_id, result);
}

static inline wl_rpc_err_t fci_arm_endpoint_set_zero_release(fci_arm_endpoint_t *endpoint, uint32_t operation_id) {
  return fci_arm_set_zero_client_release(fci_arm_endpoint_runtime(endpoint), operation_id);
}

static inline fci_arm_runtime_result_t fci_arm_endpoint_set_zero_complete(fci_arm_endpoint_t *endpoint, const wl_rpc_server_request_t *request, const set_zero_response_t *response) {
  wl_time_ms_t now_ms = 0U;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  return fci_arm_set_zero_server_complete(fci_arm_endpoint_runtime(endpoint), request, response, now_ms);
}

#ifdef __cplusplus
}
#endif

#endif
