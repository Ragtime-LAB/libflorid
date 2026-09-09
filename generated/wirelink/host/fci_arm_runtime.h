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
#include <wirelink/rpc_sync.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FCI_ARM_SCHEMA_IDENTITY UINT64_C(0xC71FA187AA9F7433)
#define FCI_ARM_BINDING_PROFILE_IDENTITY UINT64_C(0x5D53D95EABDC4FBE)
#define FCI_ARM_BINDING_PROFILE_VERSION 1U
#define FCI_ARM_IDENTITY_ALGORITHM "fnv1a64-v1"

#define FCI_ARM_RUNTIME_CODEGEN_ABI_VERSION 31U

/* Generated capabilities, not application overrides. */
#define FCI_ARM_RUNTIME_HAS_RPC_CLIENT 1
#define FCI_ARM_RUNTIME_HAS_RPC_SERVER 0

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

#define FCI_ARM_RUNTIME_HAS_MANAGED_RPC 1
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

/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_acquire_control_lease_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_acquire_control_lease_rpc_request_handler_fn)(void *user_data,
    const acquire_control_lease_request_t *request, const fci_arm_acquire_control_lease_request_token_t *token,
    wl_delivery_t delivery);

#if ACQUIRE_CONTROL_LEASE_REQUEST_HAS_VALUE && ACQUIRE_CONTROL_LEASE_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_acquire_control_lease_handler_fn)(void *user_data,
    const acquire_control_lease_request_value_t *request, acquire_control_lease_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_acquire_control_lease_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const acquire_control_lease_response_value_t *response);
#endif
typedef struct {
  acquire_control_lease_request_t *request_scratch;
  acquire_control_lease_response_t *response_scratch;
  fci_arm_acquire_control_lease_rpc_request_handler_fn request_handler;
  void *user_data;
#if ACQUIRE_CONTROL_LEASE_REQUEST_HAS_VALUE && ACQUIRE_CONTROL_LEASE_RESPONSE_HAS_VALUE
  fci_arm_acquire_control_lease_handler_fn value_handler;
  void *value_user_data;
  acquire_control_lease_request_value_t *request_value;
  acquire_control_lease_response_value_t *response_value;
#endif
} fci_arm_acquire_control_lease_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_clear_error_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_clear_error_rpc_request_handler_fn)(void *user_data,
    const clear_error_request_t *request, const fci_arm_clear_error_request_token_t *token,
    wl_delivery_t delivery);

#if CLEAR_ERROR_REQUEST_HAS_VALUE && CLEAR_ERROR_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_clear_error_handler_fn)(void *user_data,
    const clear_error_request_value_t *request, clear_error_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_clear_error_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const clear_error_response_value_t *response);
#endif
typedef struct {
  clear_error_request_t *request_scratch;
  clear_error_response_t *response_scratch;
  fci_arm_clear_error_rpc_request_handler_fn request_handler;
  void *user_data;
#if CLEAR_ERROR_REQUEST_HAS_VALUE && CLEAR_ERROR_RESPONSE_HAS_VALUE
  fci_arm_clear_error_handler_fn value_handler;
  void *value_user_data;
  clear_error_request_value_t *request_value;
  clear_error_response_value_t *response_value;
#endif
} fci_arm_clear_error_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_clear_faults_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_clear_faults_rpc_request_handler_fn)(void *user_data,
    const clear_faults_request_t *request, const fci_arm_clear_faults_request_token_t *token,
    wl_delivery_t delivery);

#if CLEAR_FAULTS_REQUEST_HAS_VALUE && CLEAR_FAULTS_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_clear_faults_handler_fn)(void *user_data,
    const clear_faults_request_value_t *request, clear_faults_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_clear_faults_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const clear_faults_response_value_t *response);
#endif
typedef struct {
  clear_faults_request_t *request_scratch;
  clear_faults_response_t *response_scratch;
  fci_arm_clear_faults_rpc_request_handler_fn request_handler;
  void *user_data;
#if CLEAR_FAULTS_REQUEST_HAS_VALUE && CLEAR_FAULTS_RESPONSE_HAS_VALUE
  fci_arm_clear_faults_handler_fn value_handler;
  void *value_user_data;
  clear_faults_request_value_t *request_value;
  clear_faults_response_value_t *response_value;
#endif
} fci_arm_clear_faults_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_emergency_stop_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_emergency_stop_rpc_request_handler_fn)(void *user_data,
    const emergency_stop_request_t *request, const fci_arm_emergency_stop_request_token_t *token,
    wl_delivery_t delivery);

#if EMERGENCY_STOP_REQUEST_HAS_VALUE && EMERGENCY_STOP_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_emergency_stop_handler_fn)(void *user_data,
    const emergency_stop_request_value_t *request, emergency_stop_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_emergency_stop_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const emergency_stop_response_value_t *response);
#endif
typedef struct {
  emergency_stop_request_t *request_scratch;
  emergency_stop_response_t *response_scratch;
  fci_arm_emergency_stop_rpc_request_handler_fn request_handler;
  void *user_data;
#if EMERGENCY_STOP_REQUEST_HAS_VALUE && EMERGENCY_STOP_RESPONSE_HAS_VALUE
  fci_arm_emergency_stop_handler_fn value_handler;
  void *value_user_data;
  emergency_stop_request_value_t *request_value;
  emergency_stop_response_value_t *response_value;
#endif
} fci_arm_emergency_stop_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_get_device_info_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_get_device_info_rpc_request_handler_fn)(void *user_data,
    const get_device_info_request_t *request, const fci_arm_get_device_info_request_token_t *token,
    wl_delivery_t delivery);

#if GET_DEVICE_INFO_REQUEST_HAS_VALUE && GET_DEVICE_INFO_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_get_device_info_handler_fn)(void *user_data,
    const get_device_info_request_value_t *request, get_device_info_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_get_device_info_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const get_device_info_response_value_t *response);
#endif
typedef struct {
  get_device_info_request_t *request_scratch;
  get_device_info_response_t *response_scratch;
  fci_arm_get_device_info_rpc_request_handler_fn request_handler;
  void *user_data;
#if GET_DEVICE_INFO_REQUEST_HAS_VALUE && GET_DEVICE_INFO_RESPONSE_HAS_VALUE
  fci_arm_get_device_info_handler_fn value_handler;
  void *value_user_data;
  get_device_info_request_value_t *request_value;
  get_device_info_response_value_t *response_value;
#endif
} fci_arm_get_device_info_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_get_device_settings_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_get_device_settings_rpc_request_handler_fn)(void *user_data,
    const get_device_settings_request_t *request, const fci_arm_get_device_settings_request_token_t *token,
    wl_delivery_t delivery);

#if GET_DEVICE_SETTINGS_REQUEST_HAS_VALUE && GET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_get_device_settings_handler_fn)(void *user_data,
    const get_device_settings_request_value_t *request, get_device_settings_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_get_device_settings_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const get_device_settings_response_value_t *response);
#endif
typedef struct {
  get_device_settings_request_t *request_scratch;
  get_device_settings_response_t *response_scratch;
  fci_arm_get_device_settings_rpc_request_handler_fn request_handler;
  void *user_data;
#if GET_DEVICE_SETTINGS_REQUEST_HAS_VALUE && GET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE
  fci_arm_get_device_settings_handler_fn value_handler;
  void *value_user_data;
  get_device_settings_request_value_t *request_value;
  get_device_settings_response_value_t *response_value;
#endif
} fci_arm_get_device_settings_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_get_motor_feedback_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_get_motor_feedback_rpc_request_handler_fn)(void *user_data,
    const get_motor_feedback_request_t *request, const fci_arm_get_motor_feedback_request_token_t *token,
    wl_delivery_t delivery);

#if GET_MOTOR_FEEDBACK_REQUEST_HAS_VALUE && GET_MOTOR_FEEDBACK_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_get_motor_feedback_handler_fn)(void *user_data,
    const get_motor_feedback_request_value_t *request, get_motor_feedback_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_get_motor_feedback_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const get_motor_feedback_response_value_t *response);
#endif
typedef struct {
  get_motor_feedback_request_t *request_scratch;
  get_motor_feedback_response_t *response_scratch;
  fci_arm_get_motor_feedback_rpc_request_handler_fn request_handler;
  void *user_data;
#if GET_MOTOR_FEEDBACK_REQUEST_HAS_VALUE && GET_MOTOR_FEEDBACK_RESPONSE_HAS_VALUE
  fci_arm_get_motor_feedback_handler_fn value_handler;
  void *value_user_data;
  get_motor_feedback_request_value_t *request_value;
  get_motor_feedback_response_value_t *response_value;
#endif
} fci_arm_get_motor_feedback_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_home_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_home_rpc_request_handler_fn)(void *user_data,
    const home_request_t *request, const fci_arm_home_request_token_t *token,
    wl_delivery_t delivery);

#if HOME_REQUEST_HAS_VALUE && HOME_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_home_handler_fn)(void *user_data,
    const home_request_value_t *request, home_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_home_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const home_response_value_t *response);
#endif
typedef struct {
  home_request_t *request_scratch;
  home_response_t *response_scratch;
  fci_arm_home_rpc_request_handler_fn request_handler;
  void *user_data;
#if HOME_REQUEST_HAS_VALUE && HOME_RESPONSE_HAS_VALUE
  fci_arm_home_handler_fn value_handler;
  void *value_user_data;
  home_request_value_t *request_value;
  home_response_value_t *response_value;
#endif
} fci_arm_home_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_motor_register_read_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_motor_register_read_rpc_request_handler_fn)(void *user_data,
    const motor_register_read_request_t *request, const fci_arm_motor_register_read_request_token_t *token,
    wl_delivery_t delivery);

#if MOTOR_REGISTER_READ_REQUEST_HAS_VALUE && MOTOR_REGISTER_READ_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_motor_register_read_handler_fn)(void *user_data,
    const motor_register_read_request_value_t *request, motor_register_read_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_motor_register_read_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const motor_register_read_response_value_t *response);
#endif
typedef struct {
  motor_register_read_request_t *request_scratch;
  motor_register_read_response_t *response_scratch;
  fci_arm_motor_register_read_rpc_request_handler_fn request_handler;
  void *user_data;
#if MOTOR_REGISTER_READ_REQUEST_HAS_VALUE && MOTOR_REGISTER_READ_RESPONSE_HAS_VALUE
  fci_arm_motor_register_read_handler_fn value_handler;
  void *value_user_data;
  motor_register_read_request_value_t *request_value;
  motor_register_read_response_value_t *response_value;
#endif
} fci_arm_motor_register_read_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_motor_register_write_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_motor_register_write_rpc_request_handler_fn)(void *user_data,
    const motor_register_write_request_t *request, const fci_arm_motor_register_write_request_token_t *token,
    wl_delivery_t delivery);

#if MOTOR_REGISTER_WRITE_REQUEST_HAS_VALUE && MOTOR_REGISTER_WRITE_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_motor_register_write_handler_fn)(void *user_data,
    const motor_register_write_request_value_t *request, motor_register_write_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_motor_register_write_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const motor_register_write_response_value_t *response);
#endif
typedef struct {
  motor_register_write_request_t *request_scratch;
  motor_register_write_response_t *response_scratch;
  fci_arm_motor_register_write_rpc_request_handler_fn request_handler;
  void *user_data;
#if MOTOR_REGISTER_WRITE_REQUEST_HAS_VALUE && MOTOR_REGISTER_WRITE_RESPONSE_HAS_VALUE
  fci_arm_motor_register_write_handler_fn value_handler;
  void *value_user_data;
  motor_register_write_request_value_t *request_value;
  motor_register_write_response_value_t *response_value;
#endif
} fci_arm_motor_register_write_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_motor_set_zero_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_motor_set_zero_rpc_request_handler_fn)(void *user_data,
    const motor_set_zero_request_t *request, const fci_arm_motor_set_zero_request_token_t *token,
    wl_delivery_t delivery);

#if MOTOR_SET_ZERO_REQUEST_HAS_VALUE && MOTOR_SET_ZERO_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_motor_set_zero_handler_fn)(void *user_data,
    const motor_set_zero_request_value_t *request, motor_set_zero_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_motor_set_zero_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const motor_set_zero_response_value_t *response);
#endif
typedef struct {
  motor_set_zero_request_t *request_scratch;
  motor_set_zero_response_t *response_scratch;
  fci_arm_motor_set_zero_rpc_request_handler_fn request_handler;
  void *user_data;
#if MOTOR_SET_ZERO_REQUEST_HAS_VALUE && MOTOR_SET_ZERO_RESPONSE_HAS_VALUE
  fci_arm_motor_set_zero_handler_fn value_handler;
  void *value_user_data;
  motor_set_zero_request_value_t *request_value;
  motor_set_zero_response_value_t *response_value;
#endif
} fci_arm_motor_set_zero_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_motor_store_parameters_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_motor_store_parameters_rpc_request_handler_fn)(void *user_data,
    const motor_store_parameters_request_t *request, const fci_arm_motor_store_parameters_request_token_t *token,
    wl_delivery_t delivery);

#if MOTOR_STORE_PARAMETERS_REQUEST_HAS_VALUE && MOTOR_STORE_PARAMETERS_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_motor_store_parameters_handler_fn)(void *user_data,
    const motor_store_parameters_request_value_t *request, motor_store_parameters_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_motor_store_parameters_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const motor_store_parameters_response_value_t *response);
#endif
typedef struct {
  motor_store_parameters_request_t *request_scratch;
  motor_store_parameters_response_t *response_scratch;
  fci_arm_motor_store_parameters_rpc_request_handler_fn request_handler;
  void *user_data;
#if MOTOR_STORE_PARAMETERS_REQUEST_HAS_VALUE && MOTOR_STORE_PARAMETERS_RESPONSE_HAS_VALUE
  fci_arm_motor_store_parameters_handler_fn value_handler;
  void *value_user_data;
  motor_store_parameters_request_value_t *request_value;
  motor_store_parameters_response_value_t *response_value;
#endif
} fci_arm_motor_store_parameters_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_release_control_lease_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_release_control_lease_rpc_request_handler_fn)(void *user_data,
    const release_control_lease_request_t *request, const fci_arm_release_control_lease_request_token_t *token,
    wl_delivery_t delivery);

#if RELEASE_CONTROL_LEASE_REQUEST_HAS_VALUE && RELEASE_CONTROL_LEASE_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_release_control_lease_handler_fn)(void *user_data,
    const release_control_lease_request_value_t *request, release_control_lease_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_release_control_lease_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const release_control_lease_response_value_t *response);
#endif
typedef struct {
  release_control_lease_request_t *request_scratch;
  release_control_lease_response_t *response_scratch;
  fci_arm_release_control_lease_rpc_request_handler_fn request_handler;
  void *user_data;
#if RELEASE_CONTROL_LEASE_REQUEST_HAS_VALUE && RELEASE_CONTROL_LEASE_RESPONSE_HAS_VALUE
  fci_arm_release_control_lease_handler_fn value_handler;
  void *value_user_data;
  release_control_lease_request_value_t *request_value;
  release_control_lease_response_value_t *response_value;
#endif
} fci_arm_release_control_lease_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_set_arm_control_mode_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_set_arm_control_mode_rpc_request_handler_fn)(void *user_data,
    const set_arm_control_mode_request_t *request, const fci_arm_set_arm_control_mode_request_token_t *token,
    wl_delivery_t delivery);

#if SET_ARM_CONTROL_MODE_REQUEST_HAS_VALUE && SET_ARM_CONTROL_MODE_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_set_arm_control_mode_handler_fn)(void *user_data,
    const set_arm_control_mode_request_value_t *request, set_arm_control_mode_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_set_arm_control_mode_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const set_arm_control_mode_response_value_t *response);
#endif
typedef struct {
  set_arm_control_mode_request_t *request_scratch;
  set_arm_control_mode_response_t *response_scratch;
  fci_arm_set_arm_control_mode_rpc_request_handler_fn request_handler;
  void *user_data;
#if SET_ARM_CONTROL_MODE_REQUEST_HAS_VALUE && SET_ARM_CONTROL_MODE_RESPONSE_HAS_VALUE
  fci_arm_set_arm_control_mode_handler_fn value_handler;
  void *value_user_data;
  set_arm_control_mode_request_value_t *request_value;
  set_arm_control_mode_response_value_t *response_value;
#endif
} fci_arm_set_arm_control_mode_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_set_arm_mode_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_set_arm_mode_rpc_request_handler_fn)(void *user_data,
    const set_arm_mode_request_t *request, const fci_arm_set_arm_mode_request_token_t *token,
    wl_delivery_t delivery);

#if SET_ARM_MODE_REQUEST_HAS_VALUE && SET_ARM_MODE_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_set_arm_mode_handler_fn)(void *user_data,
    const set_arm_mode_request_value_t *request, set_arm_mode_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_set_arm_mode_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const set_arm_mode_response_value_t *response);
#endif
typedef struct {
  set_arm_mode_request_t *request_scratch;
  set_arm_mode_response_t *response_scratch;
  fci_arm_set_arm_mode_rpc_request_handler_fn request_handler;
  void *user_data;
#if SET_ARM_MODE_REQUEST_HAS_VALUE && SET_ARM_MODE_RESPONSE_HAS_VALUE
  fci_arm_set_arm_mode_handler_fn value_handler;
  void *value_user_data;
  set_arm_mode_request_value_t *request_value;
  set_arm_mode_response_value_t *response_value;
#endif
} fci_arm_set_arm_mode_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_set_device_info_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_set_device_info_rpc_request_handler_fn)(void *user_data,
    const set_device_info_request_t *request, const fci_arm_set_device_info_request_token_t *token,
    wl_delivery_t delivery);

#if SET_DEVICE_INFO_REQUEST_HAS_VALUE && SET_DEVICE_INFO_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_set_device_info_handler_fn)(void *user_data,
    const set_device_info_request_value_t *request, set_device_info_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_set_device_info_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const set_device_info_response_value_t *response);
#endif
typedef struct {
  set_device_info_request_t *request_scratch;
  set_device_info_response_t *response_scratch;
  fci_arm_set_device_info_rpc_request_handler_fn request_handler;
  void *user_data;
#if SET_DEVICE_INFO_REQUEST_HAS_VALUE && SET_DEVICE_INFO_RESPONSE_HAS_VALUE
  fci_arm_set_device_info_handler_fn value_handler;
  void *value_user_data;
  set_device_info_request_value_t *request_value;
  set_device_info_response_value_t *response_value;
#endif
} fci_arm_set_device_info_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_set_device_settings_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_set_device_settings_rpc_request_handler_fn)(void *user_data,
    const set_device_settings_request_t *request, const fci_arm_set_device_settings_request_token_t *token,
    wl_delivery_t delivery);

#if SET_DEVICE_SETTINGS_REQUEST_HAS_VALUE && SET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_set_device_settings_handler_fn)(void *user_data,
    const set_device_settings_request_value_t *request, set_device_settings_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_set_device_settings_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const set_device_settings_response_value_t *response);
#endif
typedef struct {
  set_device_settings_request_t *request_scratch;
  set_device_settings_response_t *response_scratch;
  fci_arm_set_device_settings_rpc_request_handler_fn request_handler;
  void *user_data;
#if SET_DEVICE_SETTINGS_REQUEST_HAS_VALUE && SET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE
  fci_arm_set_device_settings_handler_fn value_handler;
  void *value_user_data;
  set_device_settings_request_value_t *request_value;
  set_device_settings_response_value_t *response_value;
#endif
} fci_arm_set_device_settings_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_set_gripper_control_mode_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_set_gripper_control_mode_rpc_request_handler_fn)(void *user_data,
    const set_gripper_control_mode_request_t *request, const fci_arm_set_gripper_control_mode_request_token_t *token,
    wl_delivery_t delivery);

#if SET_GRIPPER_CONTROL_MODE_REQUEST_HAS_VALUE && SET_GRIPPER_CONTROL_MODE_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_set_gripper_control_mode_handler_fn)(void *user_data,
    const set_gripper_control_mode_request_value_t *request, set_gripper_control_mode_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_set_gripper_control_mode_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const set_gripper_control_mode_response_value_t *response);
#endif
typedef struct {
  set_gripper_control_mode_request_t *request_scratch;
  set_gripper_control_mode_response_t *response_scratch;
  fci_arm_set_gripper_control_mode_rpc_request_handler_fn request_handler;
  void *user_data;
#if SET_GRIPPER_CONTROL_MODE_REQUEST_HAS_VALUE && SET_GRIPPER_CONTROL_MODE_RESPONSE_HAS_VALUE
  fci_arm_set_gripper_control_mode_handler_fn value_handler;
  void *value_user_data;
  set_gripper_control_mode_request_value_t *request_value;
  set_gripper_control_mode_response_value_t *response_value;
#endif
} fci_arm_set_gripper_control_mode_rpc_t;
/* Copy the request token for deferred replies. Its private state is not
 * application data; it authorizes only this runtime and execution lifetime. */
typedef struct {
  struct {
    const void *owner;
    uint64_t incarnation;
    wl_rpc_server_request_t request;
  } private_state;
} fci_arm_set_zero_request_token_t;

/* Request fields are borrowed only for this callback. Nonzero return abandons
 * execution locally; send a rejection explicitly to notify the caller. */
typedef int32_t (*fci_arm_set_zero_rpc_request_handler_fn)(void *user_data,
    const set_zero_request_t *request, const fci_arm_set_zero_request_token_t *token,
    wl_delivery_t delivery);

#if SET_ZERO_REQUEST_HAS_VALUE && SET_ZERO_RESPONSE_HAS_VALUE
/* Ordinary immediate business handler: zero succeeds, nonzero rejects with
 * that application code. response is initialized before the callback. */
typedef int32_t (*fci_arm_set_zero_handler_fn)(void *user_data,
    const set_zero_request_value_t *request, set_zero_response_value_t *response);
/* A response exists only on SUCCESS and lives through this callback. Assign
 * *response to save an independent value. No call-slot release is required. */
typedef void (*fci_arm_set_zero_completion_fn)(void *user_data,
    const wl_rpc_completion_t *result, const set_zero_response_value_t *response);
#endif
typedef struct {
  set_zero_request_t *request_scratch;
  set_zero_response_t *response_scratch;
  fci_arm_set_zero_rpc_request_handler_fn request_handler;
  void *user_data;
#if SET_ZERO_REQUEST_HAS_VALUE && SET_ZERO_RESPONSE_HAS_VALUE
  fci_arm_set_zero_handler_fn value_handler;
  void *value_user_data;
  set_zero_request_value_t *request_value;
  set_zero_response_value_t *response_value;
#endif
} fci_arm_set_zero_rpc_t;
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
  wl_tx_handle_t rpc_retiring_tx;
  uint64_t rpc_incarnation;
  wl_rpc_async_t *rpc_async;
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
   226U)

typedef union {
  fci_arm_runtime_default_storage_alignment_t alignment;
  uint8_t bytes[FCI_ARM_RUNTIME_DEFAULT_STORAGE_CAPACITY];
} fci_arm_runtime_default_storage_t;

typedef union { acquire_control_lease_response_t response; } fci_arm_runtime_acquire_control_lease_decode_detail_t;
typedef union { clear_error_response_t response; } fci_arm_runtime_clear_error_decode_detail_t;
typedef union { clear_faults_response_t response; } fci_arm_runtime_clear_faults_decode_detail_t;
typedef union { emergency_stop_response_t response; } fci_arm_runtime_emergency_stop_decode_detail_t;
typedef union { get_device_info_response_t response; } fci_arm_runtime_get_device_info_decode_detail_t;
typedef union { get_device_settings_response_t response; } fci_arm_runtime_get_device_settings_decode_detail_t;
typedef union { get_motor_feedback_response_t response; } fci_arm_runtime_get_motor_feedback_decode_detail_t;
typedef union { home_response_t response; } fci_arm_runtime_home_decode_detail_t;
typedef union { motor_register_read_response_t response; } fci_arm_runtime_motor_register_read_decode_detail_t;
typedef union { motor_register_write_response_t response; } fci_arm_runtime_motor_register_write_decode_detail_t;
typedef union { motor_set_zero_response_t response; } fci_arm_runtime_motor_set_zero_decode_detail_t;
typedef union { motor_store_parameters_response_t response; } fci_arm_runtime_motor_store_parameters_decode_detail_t;
typedef union { release_control_lease_response_t response; } fci_arm_runtime_release_control_lease_decode_detail_t;
typedef union { set_arm_control_mode_response_t response; } fci_arm_runtime_set_arm_control_mode_decode_detail_t;
typedef union { set_arm_mode_response_t response; } fci_arm_runtime_set_arm_mode_decode_detail_t;
typedef union { set_device_info_response_t response; } fci_arm_runtime_set_device_info_decode_detail_t;
typedef union { set_device_settings_response_t response; } fci_arm_runtime_set_device_settings_decode_detail_t;
typedef union { set_gripper_control_mode_response_t response; } fci_arm_runtime_set_gripper_control_mode_decode_detail_t;
typedef union { set_zero_response_t response; } fci_arm_runtime_set_zero_decode_detail_t;
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

/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const acquire_control_lease_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_acquire_control_lease_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_decode(const wl_rpc_client_result_t *client,
    acquire_control_lease_response_t *response);
wl_rpc_err_t fci_arm_acquire_control_lease_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_acquire_control_lease_request_token_t *token, const acquire_control_lease_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_acquire_control_lease_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_acquire_control_lease_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_acquire_control_lease_request_token_t *token, wl_rpc_server_request_t *out_request);
#if ACQUIRE_CONTROL_LEASE_REQUEST_HAS_VALUE && ACQUIRE_CONTROL_LEASE_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_acquire_control_lease_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_acquire_control_lease_request_token_t *token, const acquire_control_lease_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_clear_error_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const clear_error_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_clear_error_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_clear_error_client_decode(const wl_rpc_client_result_t *client,
    clear_error_response_t *response);
wl_rpc_err_t fci_arm_clear_error_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_clear_error_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_clear_error_request_token_t *token, const clear_error_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_clear_error_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_clear_error_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_clear_error_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_clear_error_request_token_t *token, wl_rpc_server_request_t *out_request);
#if CLEAR_ERROR_REQUEST_HAS_VALUE && CLEAR_ERROR_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_clear_error_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_clear_error_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_clear_error_request_token_t *token, const clear_error_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_clear_faults_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const clear_faults_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_clear_faults_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_clear_faults_client_decode(const wl_rpc_client_result_t *client,
    clear_faults_response_t *response);
wl_rpc_err_t fci_arm_clear_faults_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_clear_faults_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_clear_faults_request_token_t *token, const clear_faults_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_clear_faults_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_clear_faults_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_clear_faults_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_clear_faults_request_token_t *token, wl_rpc_server_request_t *out_request);
#if CLEAR_FAULTS_REQUEST_HAS_VALUE && CLEAR_FAULTS_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_clear_faults_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_clear_faults_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_clear_faults_request_token_t *token, const clear_faults_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_emergency_stop_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const emergency_stop_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_emergency_stop_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_emergency_stop_client_decode(const wl_rpc_client_result_t *client,
    emergency_stop_response_t *response);
wl_rpc_err_t fci_arm_emergency_stop_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_emergency_stop_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_emergency_stop_request_token_t *token, const emergency_stop_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_emergency_stop_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_emergency_stop_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_emergency_stop_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_emergency_stop_request_token_t *token, wl_rpc_server_request_t *out_request);
#if EMERGENCY_STOP_REQUEST_HAS_VALUE && EMERGENCY_STOP_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_emergency_stop_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_emergency_stop_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_emergency_stop_request_token_t *token, const emergency_stop_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_get_device_info_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const get_device_info_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_get_device_info_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_get_device_info_client_decode(const wl_rpc_client_result_t *client,
    get_device_info_response_t *response);
wl_rpc_err_t fci_arm_get_device_info_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_get_device_info_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_info_request_token_t *token, const get_device_info_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_get_device_info_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_info_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_get_device_info_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_info_request_token_t *token, wl_rpc_server_request_t *out_request);
#if GET_DEVICE_INFO_REQUEST_HAS_VALUE && GET_DEVICE_INFO_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_get_device_info_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_get_device_info_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_info_request_token_t *token, const get_device_info_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_get_device_settings_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const get_device_settings_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_get_device_settings_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_get_device_settings_client_decode(const wl_rpc_client_result_t *client,
    get_device_settings_response_t *response);
wl_rpc_err_t fci_arm_get_device_settings_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_get_device_settings_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_settings_request_token_t *token, const get_device_settings_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_get_device_settings_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_settings_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_get_device_settings_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_settings_request_token_t *token, wl_rpc_server_request_t *out_request);
#if GET_DEVICE_SETTINGS_REQUEST_HAS_VALUE && GET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_get_device_settings_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_get_device_settings_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_get_device_settings_request_token_t *token, const get_device_settings_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const get_motor_feedback_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_get_motor_feedback_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_decode(const wl_rpc_client_result_t *client,
    get_motor_feedback_response_t *response);
wl_rpc_err_t fci_arm_get_motor_feedback_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_get_motor_feedback_request_token_t *token, const get_motor_feedback_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_get_motor_feedback_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_get_motor_feedback_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_get_motor_feedback_request_token_t *token, wl_rpc_server_request_t *out_request);
#if GET_MOTOR_FEEDBACK_REQUEST_HAS_VALUE && GET_MOTOR_FEEDBACK_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_get_motor_feedback_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_get_motor_feedback_request_token_t *token, const get_motor_feedback_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_home_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const home_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_home_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_home_client_decode(const wl_rpc_client_result_t *client,
    home_response_t *response);
wl_rpc_err_t fci_arm_home_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_home_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_home_request_token_t *token, const home_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_home_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_home_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_home_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_home_request_token_t *token, wl_rpc_server_request_t *out_request);
#if HOME_REQUEST_HAS_VALUE && HOME_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_home_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_home_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_home_request_token_t *token, const home_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_motor_register_read_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const motor_register_read_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_motor_register_read_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_motor_register_read_client_decode(const wl_rpc_client_result_t *client,
    motor_register_read_response_t *response);
wl_rpc_err_t fci_arm_motor_register_read_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_motor_register_read_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_read_request_token_t *token, const motor_register_read_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_motor_register_read_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_read_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_motor_register_read_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_read_request_token_t *token, wl_rpc_server_request_t *out_request);
#if MOTOR_REGISTER_READ_REQUEST_HAS_VALUE && MOTOR_REGISTER_READ_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_motor_register_read_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_motor_register_read_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_read_request_token_t *token, const motor_register_read_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_motor_register_write_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const motor_register_write_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_motor_register_write_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_motor_register_write_client_decode(const wl_rpc_client_result_t *client,
    motor_register_write_response_t *response);
wl_rpc_err_t fci_arm_motor_register_write_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_motor_register_write_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_write_request_token_t *token, const motor_register_write_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_motor_register_write_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_write_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_motor_register_write_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_write_request_token_t *token, wl_rpc_server_request_t *out_request);
#if MOTOR_REGISTER_WRITE_REQUEST_HAS_VALUE && MOTOR_REGISTER_WRITE_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_motor_register_write_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_motor_register_write_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_motor_register_write_request_token_t *token, const motor_register_write_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_motor_set_zero_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const motor_set_zero_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_motor_set_zero_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_motor_set_zero_client_decode(const wl_rpc_client_result_t *client,
    motor_set_zero_response_t *response);
wl_rpc_err_t fci_arm_motor_set_zero_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_motor_set_zero_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_motor_set_zero_request_token_t *token, const motor_set_zero_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_motor_set_zero_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_motor_set_zero_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_motor_set_zero_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_motor_set_zero_request_token_t *token, wl_rpc_server_request_t *out_request);
#if MOTOR_SET_ZERO_REQUEST_HAS_VALUE && MOTOR_SET_ZERO_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_motor_set_zero_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_motor_set_zero_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_motor_set_zero_request_token_t *token, const motor_set_zero_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const motor_store_parameters_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_motor_store_parameters_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_decode(const wl_rpc_client_result_t *client,
    motor_store_parameters_response_t *response);
wl_rpc_err_t fci_arm_motor_store_parameters_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_motor_store_parameters_request_token_t *token, const motor_store_parameters_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_motor_store_parameters_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_motor_store_parameters_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_motor_store_parameters_request_token_t *token, wl_rpc_server_request_t *out_request);
#if MOTOR_STORE_PARAMETERS_REQUEST_HAS_VALUE && MOTOR_STORE_PARAMETERS_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_motor_store_parameters_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_motor_store_parameters_request_token_t *token, const motor_store_parameters_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_release_control_lease_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const release_control_lease_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_release_control_lease_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_release_control_lease_client_decode(const wl_rpc_client_result_t *client,
    release_control_lease_response_t *response);
wl_rpc_err_t fci_arm_release_control_lease_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_release_control_lease_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_release_control_lease_request_token_t *token, const release_control_lease_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_release_control_lease_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_release_control_lease_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_release_control_lease_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_release_control_lease_request_token_t *token, wl_rpc_server_request_t *out_request);
#if RELEASE_CONTROL_LEASE_REQUEST_HAS_VALUE && RELEASE_CONTROL_LEASE_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_release_control_lease_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_release_control_lease_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_release_control_lease_request_token_t *token, const release_control_lease_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const set_arm_control_mode_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_set_arm_control_mode_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_decode(const wl_rpc_client_result_t *client,
    set_arm_control_mode_response_t *response);
wl_rpc_err_t fci_arm_set_arm_control_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_control_mode_request_token_t *token, const set_arm_control_mode_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_control_mode_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_set_arm_control_mode_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_control_mode_request_token_t *token, wl_rpc_server_request_t *out_request);
#if SET_ARM_CONTROL_MODE_REQUEST_HAS_VALUE && SET_ARM_CONTROL_MODE_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_set_arm_control_mode_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_control_mode_request_token_t *token, const set_arm_control_mode_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_set_arm_mode_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const set_arm_mode_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_set_arm_mode_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_set_arm_mode_client_decode(const wl_rpc_client_result_t *client,
    set_arm_mode_response_t *response);
wl_rpc_err_t fci_arm_set_arm_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_set_arm_mode_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_mode_request_token_t *token, const set_arm_mode_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_set_arm_mode_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_mode_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_set_arm_mode_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_mode_request_token_t *token, wl_rpc_server_request_t *out_request);
#if SET_ARM_MODE_REQUEST_HAS_VALUE && SET_ARM_MODE_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_set_arm_mode_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_set_arm_mode_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_set_arm_mode_request_token_t *token, const set_arm_mode_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_set_device_info_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const set_device_info_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_set_device_info_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_set_device_info_client_decode(const wl_rpc_client_result_t *client,
    set_device_info_response_t *response);
wl_rpc_err_t fci_arm_set_device_info_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_set_device_info_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_info_request_token_t *token, const set_device_info_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_set_device_info_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_info_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_set_device_info_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_info_request_token_t *token, wl_rpc_server_request_t *out_request);
#if SET_DEVICE_INFO_REQUEST_HAS_VALUE && SET_DEVICE_INFO_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_set_device_info_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_set_device_info_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_info_request_token_t *token, const set_device_info_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_set_device_settings_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const set_device_settings_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_set_device_settings_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_set_device_settings_client_decode(const wl_rpc_client_result_t *client,
    set_device_settings_response_t *response);
wl_rpc_err_t fci_arm_set_device_settings_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_set_device_settings_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_settings_request_token_t *token, const set_device_settings_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_set_device_settings_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_settings_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_set_device_settings_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_settings_request_token_t *token, wl_rpc_server_request_t *out_request);
#if SET_DEVICE_SETTINGS_REQUEST_HAS_VALUE && SET_DEVICE_SETTINGS_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_set_device_settings_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_set_device_settings_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_set_device_settings_request_token_t *token, const set_device_settings_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const set_gripper_control_mode_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_set_gripper_control_mode_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_decode(const wl_rpc_client_result_t *client,
    set_gripper_control_mode_response_t *response);
wl_rpc_err_t fci_arm_set_gripper_control_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_set_gripper_control_mode_request_token_t *token, const set_gripper_control_mode_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_set_gripper_control_mode_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_set_gripper_control_mode_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_set_gripper_control_mode_request_token_t *token, wl_rpc_server_request_t *out_request);
#if SET_GRIPPER_CONTROL_MODE_REQUEST_HAS_VALUE && SET_GRIPPER_CONTROL_MODE_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_set_gripper_control_mode_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_set_gripper_control_mode_request_token_t *token, const set_gripper_control_mode_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Advanced runtime integration. The default endpoint call API below hides
 * numeric correlation IDs. Managed metadata uses a separate 20-byte header. */
fci_arm_runtime_result_t fci_arm_set_zero_client_start(wl_ctx_t *ctx,
    fci_arm_runtime_t *runtime, const set_zero_request_t *request, uint32_t timeout_ms,
    wl_time_ms_t now_ms);
wl_rpc_err_t fci_arm_set_zero_client_inspect(const fci_arm_runtime_t *runtime,
    uint32_t operation_id, wl_rpc_client_result_t *out_client);
/* Response fields which borrow bytes remain valid until call release. */
fci_arm_runtime_result_t fci_arm_set_zero_client_decode(const wl_rpc_client_result_t *client,
    set_zero_response_t *response);
wl_rpc_err_t fci_arm_set_zero_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id);
fci_arm_runtime_result_t fci_arm_set_zero_server_complete(fci_arm_runtime_t *runtime,
    const fci_arm_set_zero_request_token_t *token, const set_zero_response_t *response,
    wl_time_ms_t now_ms);
/* A nonzero business status is sent without a response body. */
fci_arm_runtime_result_t fci_arm_set_zero_server_reject(fci_arm_runtime_t *runtime,
    const fci_arm_set_zero_request_token_t *token, int32_t application_status,
    wl_time_ms_t now_ms);

/* Advanced scheduling/diagnostics: inspect a token from this runtime incarnation.
 * Not a liveness test: complete/reject still validate the pending reservation.
 * Does not grant permission to reconstruct tokens or extend their lifetime. */
wl_rpc_err_t fci_arm_set_zero_request_inspect(fci_arm_runtime_t *runtime,
    const fci_arm_set_zero_request_token_t *token, wl_rpc_server_request_t *out_request);
#if SET_ZERO_REQUEST_HAS_VALUE && SET_ZERO_RESPONSE_HAS_VALUE
/* Generator-owned snapshot/response helpers. Prefer endpoint operations. */
wl_err_t fci_arm_set_zero_encode_submission(const void *request, uint32_t operation_id, uint64_t session,
    uint8_t *out, size_t capacity, size_t *length);
fci_arm_runtime_result_t fci_arm_set_zero_server_complete_value(fci_arm_runtime_t *runtime,
    const fci_arm_set_zero_request_token_t *token, const set_zero_response_value_t *response,
    wl_time_ms_t now_ms);
#endif
/* Default endpoint assembly. Members named private_state are not application
 * API. Zero-initialize once, keep at a stable address, close before reuse.
 * All profile-selected messages must have finite one-frame bounds. */
#define FCI_ARM_HAS_DEFAULT_ENDPOINT 1
#define FCI_ARM_ENDPOINT_MAX_PAYLOAD 233U
/* Layout is fixed by the local profile; CRC32C bounds also cover smaller CRCs. */
#define FCI_ARM_ENDPOINT_RAW_CAPACITY (FCI_ARM_ENDPOINT_MAX_PAYLOAD + WL_FRAME_HEADER_SIZE + WL_FRAME_MAX_CRC)
#define FCI_ARM_ENDPOINT_UNIT_CAPACITY (FCI_ARM_ENDPOINT_RAW_CAPACITY + FCI_ARM_ENDPOINT_RAW_CAPACITY / 254U + 2U)
#define FCI_ARM_ENDPOINT_CONTROL_CAPACITY (WL_FRAME_HEADER_SIZE + WL_FRAME_MAX_CRC + 2U)
#ifndef FCI_ARM_ENDPOINT_RX_FIFO_CAPACITY
#define FCI_ARM_ENDPOINT_RX_FIFO_CAPACITY FCI_ARM_ENDPOINT_UNIT_CAPACITY
#endif
/* Set consistently for every TU using this endpoint; no runtime allocation. */
#ifndef FCI_ARM_ENDPOINT_RPC_CAPACITY
#define FCI_ARM_ENDPOINT_RPC_CAPACITY 4U
#endif
#if FCI_ARM_ENDPOINT_RPC_CAPACITY < 1 || FCI_ARM_ENDPOINT_RPC_CAPACITY > 65535
#error "endpoint RPC capacity must be 1..65535"
#endif
#define FCI_ARM_ENDPOINT_REQUEST_CAPACITY 226U
#define FCI_ARM_ENDPOINT_RUNTIME_CAPACITY (FCI_ARM_RUNTIME_DEFAULT_STORAGE_CAPACITY + (FCI_ARM_ENDPOINT_RPC_CAPACITY - 1U) * (sizeof(wl_rpc_client_slot_t) + 226U))

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
    uint64_t incarnation;
    bool sync_waiting;
    wl_rpc_async_t async;
    wl_rpc_async_slot_t submissions[FCI_ARM_ENDPOINT_RPC_CAPACITY];
    uint8_t requests[FCI_ARM_ENDPOINT_RPC_CAPACITY][FCI_ARM_ENDPOINT_REQUEST_CAPACITY];
    wl_rpc_completion_t completion;
    union {
      struct { acquire_control_lease_response_value_t response; } acquire_control_lease;
      struct { clear_error_response_value_t response; } clear_error;
      struct { clear_faults_response_value_t response; } clear_faults;
      struct { emergency_stop_response_value_t response; } emergency_stop;
      struct { get_device_info_response_value_t response; } get_device_info;
      struct { get_device_settings_response_value_t response; } get_device_settings;
      struct { get_motor_feedback_response_value_t response; } get_motor_feedback;
      struct { home_response_value_t response; } home;
      struct { motor_register_read_response_value_t response; } motor_register_read;
      struct { motor_register_write_response_value_t response; } motor_register_write;
      struct { motor_set_zero_response_value_t response; } motor_set_zero;
      struct { motor_store_parameters_response_value_t response; } motor_store_parameters;
      struct { release_control_lease_response_value_t response; } release_control_lease;
      struct { set_arm_control_mode_response_value_t response; } set_arm_control_mode;
      struct { set_arm_mode_response_value_t response; } set_arm_mode;
      struct { set_device_info_response_value_t response; } set_device_info;
      struct { set_device_settings_response_value_t response; } set_device_settings;
      struct { set_gripper_control_mode_response_value_t response; } set_gripper_control_mode;
      struct { set_zero_response_value_t response; } set_zero;
    } values;
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
  config->link.envelope = WL_ENVELOPE_COBS_STREAM;
  config->link.integrity = WL_INTEGRITY_CRC32C;
  config->environment = environment;
  config->link.ack_timeout_ms = 100U;
  config->link.max_retries = 4U;
  config->event_budget = 16U;
  result = fci_arm_runtime_config_defaults(&config->advanced);
  if (result != WL_OK) return result;
  config->advanced.rpc_client_enabled = 1U;
  config->advanced.rpc_client_slot_count = FCI_ARM_ENDPOINT_RPC_CAPACITY;
  config->advanced.rpc_server_pending_slot_count = FCI_ARM_ENDPOINT_RPC_CAPACITY;
  config->advanced.rpc_server_cache_slot_count = FCI_ARM_ENDPOINT_RPC_CAPACITY;
  config->advanced.rpc_server_pending_timeout_ms = 1000U;
  config->advanced.rpc_server_cache_ttl_ms = 10000U;
  config->advanced.rpc_server_cache_policy = WL_RPC_CACHE_EVICT_OLDEST;
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
  if (config->link.envelope != WL_ENVELOPE_COBS_STREAM) return WL_ERR_NOT_SUPPORTED;
  runtime_config = config->advanced;
  if (runtime_config.rpc_server_enabled) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.acquire_control_lease_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.clear_error_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.clear_faults_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.emergency_stop_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.get_device_info_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.get_device_settings_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.get_motor_feedback_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.home_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.motor_register_read_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.motor_register_write_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.motor_set_zero_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.motor_store_parameters_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.release_control_lease_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.set_arm_control_mode_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.set_arm_mode_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.set_device_info_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.set_device_settings_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.set_gripper_control_mode_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (runtime_config.set_zero_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (endpoint->private_state.closing) return WL_ERR_REENTRANT;
  if ((runtime_config.rpc_client_enabled && runtime_config.rpc_client_slot_count > FCI_ARM_ENDPOINT_RPC_CAPACITY) ||
      (runtime_config.rpc_server_enabled && (runtime_config.rpc_server_pending_slot_count > FCI_ARM_ENDPOINT_RPC_CAPACITY ||
      runtime_config.rpc_server_cache_slot_count > FCI_ARM_ENDPOINT_RPC_CAPACITY))) return WL_ERR_INVALID_ARG;

  link_config = config->link;
  result = wl_session_next(config->environment.session,
      endpoint->private_state.previous_session, &link_config.session_id);
  if (result != WL_OK) return result;
  if (endpoint->private_state.incarnation == UINT64_MAX) return WL_ERR_INVALID_STATE;
  ++endpoint->private_state.incarnation;
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
  endpoint->private_state.instance.runtime.rpc_incarnation = endpoint->private_state.incarnation;

  result = fci_arm_runtime_pump_init(&endpoint->private_state.pump,
      &endpoint->private_state.instance.runtime, fci_arm_endpoint_record, endpoint);
  if (result != WL_OK) return result;
  hooks = fci_arm_runtime_pump_hooks(&endpoint->private_state.pump);
  result = wl_endpoint_init(&endpoint->private_state.owner, &link_config,
                            &link_storage, &config->environment.clock, &hooks);
  if (result != WL_OK) return result;
  endpoint->private_state.previous_session = link_config.session_id;
  if (runtime_config.rpc_client_enabled) {
    result = wl_rpc_async_init(&endpoint->private_state.async,
        wl_endpoint_link(&endpoint->private_state.owner), endpoint->private_state.instance.runtime.rpc_client,
        endpoint->private_state.submissions, runtime_config.rpc_client_slot_count,
        endpoint->private_state.requests[0], sizeof(endpoint->private_state.requests),
        FCI_ARM_ENDPOINT_REQUEST_CAPACITY, endpoint->private_state.incarnation);
    if (result != WL_OK) { wl_endpoint_close(&endpoint->private_state.owner); return result; }
    endpoint->private_state.instance.runtime.rpc_async = &endpoint->private_state.async;
  }
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
  {
    int error = wl_rpc_async_close(&endpoint->private_state.async);
    endpoint->private_state.closing = false;
    if (error != WL_OK) return error;
  }
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
  driver.readiness_complete = 1U;
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

/* Optional cancellation; completion still arrives once, with no release. */
static inline wl_err_t fci_arm_endpoint_cancel(fci_arm_endpoint_t *endpoint, const wl_rpc_call_t *call) {
  if (endpoint == NULL || call == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing || wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) return WL_ERR_NOT_INITIALIZED;
  return wl_rpc_async_cancel(&endpoint->private_state.async, call);
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

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_joint_mit_command(fci_arm_endpoint_t *endpoint, const joint_mit_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_joint_mit_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_gripper_mit_command(fci_arm_endpoint_t *endpoint, const gripper_mit_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_gripper_mit_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_joint_position_velocity_command(fci_arm_endpoint_t *endpoint, const joint_position_velocity_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_joint_position_velocity_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_joint_velocity_command(fci_arm_endpoint_t *endpoint, const joint_velocity_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_joint_velocity_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_joint_pvt_command(fci_arm_endpoint_t *endpoint, const joint_pvt_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_joint_pvt_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_cartesian_pose_command(fci_arm_endpoint_t *endpoint, const cartesian_pose_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_cartesian_pose_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_cartesian_velocity_command(fci_arm_endpoint_t *endpoint, const cartesian_velocity_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_cartesian_velocity_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_gripper_position_velocity_command(fci_arm_endpoint_t *endpoint, const gripper_position_velocity_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_gripper_position_velocity_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_gripper_velocity_command(fci_arm_endpoint_t *endpoint, const gripper_velocity_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_gripper_velocity_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Delivery follows this binding. Use codec sends to override explicitly. */
static inline fci_arm_send_result_t fci_arm_endpoint_send_gripper_pvt_command(fci_arm_endpoint_t *endpoint, const gripper_pvt_command_t *message) {
  wl_time_ms_t now_ms = 0U;
  return fci_arm_gripper_pvt_command_send(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), message, WL_DELIVERY_UNRELIABLE, now_ms);
}

/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_acquire_control_lease_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = acquire_control_lease_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.acquire_control_lease.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_acquire_control_lease_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_acquire_control_lease_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.acquire_control_lease.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_acquire_control_lease_submit_at(fci_arm_endpoint_t *endpoint,
    const acquire_control_lease_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_acquire_control_lease_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_acquire_control_lease_prepare;
  observer.notify = fci_arm_endpoint_acquire_control_lease_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25097U, 25098U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_acquire_control_lease_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_acquire_control_lease_async(fci_arm_endpoint_t *endpoint,
    const acquire_control_lease_request_value_t *request, uint32_t timeout_ms,
    fci_arm_acquire_control_lease_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_acquire_control_lease_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  acquire_control_lease_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const acquire_control_lease_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_acquire_control_lease_sync_state_t;

static inline void fci_arm_endpoint_acquire_control_lease_sync_done(void *context,
    const wl_rpc_completion_t *result, const acquire_control_lease_response_value_t *response) {
  fci_arm_acquire_control_lease_sync_state_t *state = (fci_arm_acquire_control_lease_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_acquire_control_lease_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_acquire_control_lease_sync_state_t *state = (fci_arm_acquire_control_lease_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_acquire_control_lease_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_acquire_control_lease_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_acquire_control_lease_sync(fci_arm_endpoint_t *endpoint,
    const acquire_control_lease_request_value_t *request, acquire_control_lease_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_acquire_control_lease_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_acquire_control_lease_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_acquire_control_lease_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_acquire_control_lease_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_clear_error_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = clear_error_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.clear_error.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_clear_error_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_clear_error_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.clear_error.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_clear_error_submit_at(fci_arm_endpoint_t *endpoint,
    const clear_error_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_clear_error_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_clear_error_prepare;
  observer.notify = fci_arm_endpoint_clear_error_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 24841U, 24842U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_clear_error_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_clear_error_async(fci_arm_endpoint_t *endpoint,
    const clear_error_request_value_t *request, uint32_t timeout_ms,
    fci_arm_clear_error_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_clear_error_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  clear_error_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const clear_error_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_clear_error_sync_state_t;

static inline void fci_arm_endpoint_clear_error_sync_done(void *context,
    const wl_rpc_completion_t *result, const clear_error_response_value_t *response) {
  fci_arm_clear_error_sync_state_t *state = (fci_arm_clear_error_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_clear_error_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_clear_error_sync_state_t *state = (fci_arm_clear_error_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_clear_error_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_clear_error_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_clear_error_sync(fci_arm_endpoint_t *endpoint,
    const clear_error_request_value_t *request, clear_error_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_clear_error_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_clear_error_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_clear_error_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_clear_error_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_clear_faults_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = clear_faults_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.clear_faults.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_clear_faults_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_clear_faults_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.clear_faults.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_clear_faults_submit_at(fci_arm_endpoint_t *endpoint,
    const clear_faults_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_clear_faults_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_clear_faults_prepare;
  observer.notify = fci_arm_endpoint_clear_faults_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25093U, 25094U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_clear_faults_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_clear_faults_async(fci_arm_endpoint_t *endpoint,
    const clear_faults_request_value_t *request, uint32_t timeout_ms,
    fci_arm_clear_faults_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_clear_faults_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  clear_faults_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const clear_faults_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_clear_faults_sync_state_t;

static inline void fci_arm_endpoint_clear_faults_sync_done(void *context,
    const wl_rpc_completion_t *result, const clear_faults_response_value_t *response) {
  fci_arm_clear_faults_sync_state_t *state = (fci_arm_clear_faults_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_clear_faults_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_clear_faults_sync_state_t *state = (fci_arm_clear_faults_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_clear_faults_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_clear_faults_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_clear_faults_sync(fci_arm_endpoint_t *endpoint,
    const clear_faults_request_value_t *request, clear_faults_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_clear_faults_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_clear_faults_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_clear_faults_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_clear_faults_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_emergency_stop_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = emergency_stop_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.emergency_stop.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_emergency_stop_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_emergency_stop_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.emergency_stop.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_emergency_stop_submit_at(fci_arm_endpoint_t *endpoint,
    const emergency_stop_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_emergency_stop_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_emergency_stop_prepare;
  observer.notify = fci_arm_endpoint_emergency_stop_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25347U, 25348U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_emergency_stop_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_emergency_stop_async(fci_arm_endpoint_t *endpoint,
    const emergency_stop_request_value_t *request, uint32_t timeout_ms,
    fci_arm_emergency_stop_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_emergency_stop_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  emergency_stop_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const emergency_stop_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_emergency_stop_sync_state_t;

static inline void fci_arm_endpoint_emergency_stop_sync_done(void *context,
    const wl_rpc_completion_t *result, const emergency_stop_response_value_t *response) {
  fci_arm_emergency_stop_sync_state_t *state = (fci_arm_emergency_stop_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_emergency_stop_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_emergency_stop_sync_state_t *state = (fci_arm_emergency_stop_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_emergency_stop_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_emergency_stop_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_emergency_stop_sync(fci_arm_endpoint_t *endpoint,
    const emergency_stop_request_value_t *request, emergency_stop_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_emergency_stop_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_emergency_stop_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_emergency_stop_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_emergency_stop_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_get_device_info_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = get_device_info_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.get_device_info.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_get_device_info_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_get_device_info_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.get_device_info.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_get_device_info_submit_at(fci_arm_endpoint_t *endpoint,
    const get_device_info_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_get_device_info_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_get_device_info_prepare;
  observer.notify = fci_arm_endpoint_get_device_info_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25109U, 25110U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_get_device_info_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_get_device_info_async(fci_arm_endpoint_t *endpoint,
    const get_device_info_request_value_t *request, uint32_t timeout_ms,
    fci_arm_get_device_info_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_get_device_info_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  get_device_info_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const get_device_info_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_get_device_info_sync_state_t;

static inline void fci_arm_endpoint_get_device_info_sync_done(void *context,
    const wl_rpc_completion_t *result, const get_device_info_response_value_t *response) {
  fci_arm_get_device_info_sync_state_t *state = (fci_arm_get_device_info_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_get_device_info_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_get_device_info_sync_state_t *state = (fci_arm_get_device_info_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_get_device_info_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_get_device_info_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_get_device_info_sync(fci_arm_endpoint_t *endpoint,
    const get_device_info_request_value_t *request, get_device_info_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_get_device_info_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_get_device_info_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_get_device_info_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_get_device_info_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_get_device_settings_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = get_device_settings_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.get_device_settings.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_get_device_settings_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_get_device_settings_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.get_device_settings.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_get_device_settings_submit_at(fci_arm_endpoint_t *endpoint,
    const get_device_settings_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_get_device_settings_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_get_device_settings_prepare;
  observer.notify = fci_arm_endpoint_get_device_settings_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25128U, 25129U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_get_device_settings_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_get_device_settings_async(fci_arm_endpoint_t *endpoint,
    const get_device_settings_request_value_t *request, uint32_t timeout_ms,
    fci_arm_get_device_settings_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_get_device_settings_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  get_device_settings_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const get_device_settings_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_get_device_settings_sync_state_t;

static inline void fci_arm_endpoint_get_device_settings_sync_done(void *context,
    const wl_rpc_completion_t *result, const get_device_settings_response_value_t *response) {
  fci_arm_get_device_settings_sync_state_t *state = (fci_arm_get_device_settings_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_get_device_settings_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_get_device_settings_sync_state_t *state = (fci_arm_get_device_settings_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_get_device_settings_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_get_device_settings_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_get_device_settings_sync(fci_arm_endpoint_t *endpoint,
    const get_device_settings_request_value_t *request, get_device_settings_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_get_device_settings_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_get_device_settings_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_get_device_settings_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_get_device_settings_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_get_motor_feedback_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = get_motor_feedback_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.get_motor_feedback.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_get_motor_feedback_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_get_motor_feedback_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.get_motor_feedback.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_get_motor_feedback_submit_at(fci_arm_endpoint_t *endpoint,
    const get_motor_feedback_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_get_motor_feedback_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_get_motor_feedback_prepare;
  observer.notify = fci_arm_endpoint_get_motor_feedback_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25107U, 25108U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_get_motor_feedback_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_get_motor_feedback_async(fci_arm_endpoint_t *endpoint,
    const get_motor_feedback_request_value_t *request, uint32_t timeout_ms,
    fci_arm_get_motor_feedback_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_get_motor_feedback_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  get_motor_feedback_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const get_motor_feedback_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_get_motor_feedback_sync_state_t;

static inline void fci_arm_endpoint_get_motor_feedback_sync_done(void *context,
    const wl_rpc_completion_t *result, const get_motor_feedback_response_value_t *response) {
  fci_arm_get_motor_feedback_sync_state_t *state = (fci_arm_get_motor_feedback_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_get_motor_feedback_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_get_motor_feedback_sync_state_t *state = (fci_arm_get_motor_feedback_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_get_motor_feedback_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_get_motor_feedback_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_get_motor_feedback_sync(fci_arm_endpoint_t *endpoint,
    const get_motor_feedback_request_value_t *request, get_motor_feedback_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_get_motor_feedback_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_get_motor_feedback_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_get_motor_feedback_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_get_motor_feedback_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_home_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = home_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.home.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_home_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_home_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.home.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_home_submit_at(fci_arm_endpoint_t *endpoint,
    const home_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_home_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_home_prepare;
  observer.notify = fci_arm_endpoint_home_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25089U, 25090U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_home_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_home_async(fci_arm_endpoint_t *endpoint,
    const home_request_value_t *request, uint32_t timeout_ms,
    fci_arm_home_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_home_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  home_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const home_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_home_sync_state_t;

static inline void fci_arm_endpoint_home_sync_done(void *context,
    const wl_rpc_completion_t *result, const home_response_value_t *response) {
  fci_arm_home_sync_state_t *state = (fci_arm_home_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_home_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_home_sync_state_t *state = (fci_arm_home_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_home_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_home_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_home_sync(fci_arm_endpoint_t *endpoint,
    const home_request_value_t *request, home_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_home_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_home_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_home_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_home_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_motor_register_read_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = motor_register_read_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.motor_register_read.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_motor_register_read_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_motor_register_read_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.motor_register_read.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_motor_register_read_submit_at(fci_arm_endpoint_t *endpoint,
    const motor_register_read_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_motor_register_read_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_motor_register_read_prepare;
  observer.notify = fci_arm_endpoint_motor_register_read_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25117U, 25118U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_motor_register_read_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_motor_register_read_async(fci_arm_endpoint_t *endpoint,
    const motor_register_read_request_value_t *request, uint32_t timeout_ms,
    fci_arm_motor_register_read_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_motor_register_read_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  motor_register_read_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const motor_register_read_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_motor_register_read_sync_state_t;

static inline void fci_arm_endpoint_motor_register_read_sync_done(void *context,
    const wl_rpc_completion_t *result, const motor_register_read_response_value_t *response) {
  fci_arm_motor_register_read_sync_state_t *state = (fci_arm_motor_register_read_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_motor_register_read_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_motor_register_read_sync_state_t *state = (fci_arm_motor_register_read_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_motor_register_read_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_motor_register_read_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_motor_register_read_sync(fci_arm_endpoint_t *endpoint,
    const motor_register_read_request_value_t *request, motor_register_read_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_motor_register_read_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_motor_register_read_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_motor_register_read_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_motor_register_read_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_motor_register_write_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = motor_register_write_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.motor_register_write.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_motor_register_write_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_motor_register_write_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.motor_register_write.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_motor_register_write_submit_at(fci_arm_endpoint_t *endpoint,
    const motor_register_write_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_motor_register_write_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_motor_register_write_prepare;
  observer.notify = fci_arm_endpoint_motor_register_write_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25119U, 25120U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_motor_register_write_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_motor_register_write_async(fci_arm_endpoint_t *endpoint,
    const motor_register_write_request_value_t *request, uint32_t timeout_ms,
    fci_arm_motor_register_write_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_motor_register_write_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  motor_register_write_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const motor_register_write_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_motor_register_write_sync_state_t;

static inline void fci_arm_endpoint_motor_register_write_sync_done(void *context,
    const wl_rpc_completion_t *result, const motor_register_write_response_value_t *response) {
  fci_arm_motor_register_write_sync_state_t *state = (fci_arm_motor_register_write_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_motor_register_write_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_motor_register_write_sync_state_t *state = (fci_arm_motor_register_write_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_motor_register_write_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_motor_register_write_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_motor_register_write_sync(fci_arm_endpoint_t *endpoint,
    const motor_register_write_request_value_t *request, motor_register_write_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_motor_register_write_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_motor_register_write_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_motor_register_write_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_motor_register_write_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_motor_set_zero_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = motor_set_zero_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.motor_set_zero.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_motor_set_zero_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_motor_set_zero_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.motor_set_zero.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_motor_set_zero_submit_at(fci_arm_endpoint_t *endpoint,
    const motor_set_zero_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_motor_set_zero_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_motor_set_zero_prepare;
  observer.notify = fci_arm_endpoint_motor_set_zero_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25123U, 25124U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_motor_set_zero_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_motor_set_zero_async(fci_arm_endpoint_t *endpoint,
    const motor_set_zero_request_value_t *request, uint32_t timeout_ms,
    fci_arm_motor_set_zero_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_motor_set_zero_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  motor_set_zero_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const motor_set_zero_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_motor_set_zero_sync_state_t;

static inline void fci_arm_endpoint_motor_set_zero_sync_done(void *context,
    const wl_rpc_completion_t *result, const motor_set_zero_response_value_t *response) {
  fci_arm_motor_set_zero_sync_state_t *state = (fci_arm_motor_set_zero_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_motor_set_zero_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_motor_set_zero_sync_state_t *state = (fci_arm_motor_set_zero_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_motor_set_zero_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_motor_set_zero_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_motor_set_zero_sync(fci_arm_endpoint_t *endpoint,
    const motor_set_zero_request_value_t *request, motor_set_zero_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_motor_set_zero_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_motor_set_zero_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_motor_set_zero_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_motor_set_zero_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_motor_store_parameters_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = motor_store_parameters_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.motor_store_parameters.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_motor_store_parameters_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_motor_store_parameters_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.motor_store_parameters.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_motor_store_parameters_submit_at(fci_arm_endpoint_t *endpoint,
    const motor_store_parameters_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_motor_store_parameters_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_motor_store_parameters_prepare;
  observer.notify = fci_arm_endpoint_motor_store_parameters_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25121U, 25122U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_motor_store_parameters_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_motor_store_parameters_async(fci_arm_endpoint_t *endpoint,
    const motor_store_parameters_request_value_t *request, uint32_t timeout_ms,
    fci_arm_motor_store_parameters_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_motor_store_parameters_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  motor_store_parameters_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const motor_store_parameters_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_motor_store_parameters_sync_state_t;

static inline void fci_arm_endpoint_motor_store_parameters_sync_done(void *context,
    const wl_rpc_completion_t *result, const motor_store_parameters_response_value_t *response) {
  fci_arm_motor_store_parameters_sync_state_t *state = (fci_arm_motor_store_parameters_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_motor_store_parameters_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_motor_store_parameters_sync_state_t *state = (fci_arm_motor_store_parameters_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_motor_store_parameters_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_motor_store_parameters_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_motor_store_parameters_sync(fci_arm_endpoint_t *endpoint,
    const motor_store_parameters_request_value_t *request, motor_store_parameters_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_motor_store_parameters_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_motor_store_parameters_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_motor_store_parameters_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_motor_store_parameters_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_release_control_lease_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = release_control_lease_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.release_control_lease.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_release_control_lease_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_release_control_lease_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.release_control_lease.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_release_control_lease_submit_at(fci_arm_endpoint_t *endpoint,
    const release_control_lease_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_release_control_lease_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_release_control_lease_prepare;
  observer.notify = fci_arm_endpoint_release_control_lease_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25099U, 25100U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_release_control_lease_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_release_control_lease_async(fci_arm_endpoint_t *endpoint,
    const release_control_lease_request_value_t *request, uint32_t timeout_ms,
    fci_arm_release_control_lease_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_release_control_lease_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  release_control_lease_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const release_control_lease_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_release_control_lease_sync_state_t;

static inline void fci_arm_endpoint_release_control_lease_sync_done(void *context,
    const wl_rpc_completion_t *result, const release_control_lease_response_value_t *response) {
  fci_arm_release_control_lease_sync_state_t *state = (fci_arm_release_control_lease_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_release_control_lease_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_release_control_lease_sync_state_t *state = (fci_arm_release_control_lease_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_release_control_lease_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_release_control_lease_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_release_control_lease_sync(fci_arm_endpoint_t *endpoint,
    const release_control_lease_request_value_t *request, release_control_lease_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_release_control_lease_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_release_control_lease_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_release_control_lease_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_release_control_lease_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_set_arm_control_mode_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = set_arm_control_mode_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.set_arm_control_mode.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_set_arm_control_mode_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_set_arm_control_mode_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.set_arm_control_mode.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_set_arm_control_mode_submit_at(fci_arm_endpoint_t *endpoint,
    const set_arm_control_mode_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_set_arm_control_mode_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_set_arm_control_mode_prepare;
  observer.notify = fci_arm_endpoint_set_arm_control_mode_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25113U, 25114U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_set_arm_control_mode_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_set_arm_control_mode_async(fci_arm_endpoint_t *endpoint,
    const set_arm_control_mode_request_value_t *request, uint32_t timeout_ms,
    fci_arm_set_arm_control_mode_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_set_arm_control_mode_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  set_arm_control_mode_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const set_arm_control_mode_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_set_arm_control_mode_sync_state_t;

static inline void fci_arm_endpoint_set_arm_control_mode_sync_done(void *context,
    const wl_rpc_completion_t *result, const set_arm_control_mode_response_value_t *response) {
  fci_arm_set_arm_control_mode_sync_state_t *state = (fci_arm_set_arm_control_mode_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_set_arm_control_mode_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_set_arm_control_mode_sync_state_t *state = (fci_arm_set_arm_control_mode_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_set_arm_control_mode_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_set_arm_control_mode_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_set_arm_control_mode_sync(fci_arm_endpoint_t *endpoint,
    const set_arm_control_mode_request_value_t *request, set_arm_control_mode_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_set_arm_control_mode_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_set_arm_control_mode_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_set_arm_control_mode_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_set_arm_control_mode_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_set_arm_mode_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = set_arm_mode_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.set_arm_mode.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_set_arm_mode_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_set_arm_mode_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.set_arm_mode.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_set_arm_mode_submit_at(fci_arm_endpoint_t *endpoint,
    const set_arm_mode_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_set_arm_mode_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_set_arm_mode_prepare;
  observer.notify = fci_arm_endpoint_set_arm_mode_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25125U, 25126U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_set_arm_mode_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_set_arm_mode_async(fci_arm_endpoint_t *endpoint,
    const set_arm_mode_request_value_t *request, uint32_t timeout_ms,
    fci_arm_set_arm_mode_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_set_arm_mode_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  set_arm_mode_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const set_arm_mode_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_set_arm_mode_sync_state_t;

static inline void fci_arm_endpoint_set_arm_mode_sync_done(void *context,
    const wl_rpc_completion_t *result, const set_arm_mode_response_value_t *response) {
  fci_arm_set_arm_mode_sync_state_t *state = (fci_arm_set_arm_mode_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_set_arm_mode_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_set_arm_mode_sync_state_t *state = (fci_arm_set_arm_mode_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_set_arm_mode_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_set_arm_mode_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_set_arm_mode_sync(fci_arm_endpoint_t *endpoint,
    const set_arm_mode_request_value_t *request, set_arm_mode_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_set_arm_mode_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_set_arm_mode_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_set_arm_mode_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_set_arm_mode_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_set_device_info_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = set_device_info_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.set_device_info.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_set_device_info_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_set_device_info_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.set_device_info.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_set_device_info_submit_at(fci_arm_endpoint_t *endpoint,
    const set_device_info_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_set_device_info_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_set_device_info_prepare;
  observer.notify = fci_arm_endpoint_set_device_info_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25111U, 25112U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_set_device_info_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_set_device_info_async(fci_arm_endpoint_t *endpoint,
    const set_device_info_request_value_t *request, uint32_t timeout_ms,
    fci_arm_set_device_info_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_set_device_info_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  set_device_info_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const set_device_info_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_set_device_info_sync_state_t;

static inline void fci_arm_endpoint_set_device_info_sync_done(void *context,
    const wl_rpc_completion_t *result, const set_device_info_response_value_t *response) {
  fci_arm_set_device_info_sync_state_t *state = (fci_arm_set_device_info_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_set_device_info_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_set_device_info_sync_state_t *state = (fci_arm_set_device_info_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_set_device_info_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_set_device_info_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_set_device_info_sync(fci_arm_endpoint_t *endpoint,
    const set_device_info_request_value_t *request, set_device_info_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_set_device_info_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_set_device_info_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_set_device_info_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_set_device_info_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_set_device_settings_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = set_device_settings_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.set_device_settings.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_set_device_settings_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_set_device_settings_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.set_device_settings.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_set_device_settings_submit_at(fci_arm_endpoint_t *endpoint,
    const set_device_settings_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_set_device_settings_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_set_device_settings_prepare;
  observer.notify = fci_arm_endpoint_set_device_settings_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25130U, 25131U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_set_device_settings_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_set_device_settings_async(fci_arm_endpoint_t *endpoint,
    const set_device_settings_request_value_t *request, uint32_t timeout_ms,
    fci_arm_set_device_settings_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_set_device_settings_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  set_device_settings_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const set_device_settings_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_set_device_settings_sync_state_t;

static inline void fci_arm_endpoint_set_device_settings_sync_done(void *context,
    const wl_rpc_completion_t *result, const set_device_settings_response_value_t *response) {
  fci_arm_set_device_settings_sync_state_t *state = (fci_arm_set_device_settings_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_set_device_settings_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_set_device_settings_sync_state_t *state = (fci_arm_set_device_settings_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_set_device_settings_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_set_device_settings_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_set_device_settings_sync(fci_arm_endpoint_t *endpoint,
    const set_device_settings_request_value_t *request, set_device_settings_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_set_device_settings_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_set_device_settings_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_set_device_settings_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_set_device_settings_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_set_gripper_control_mode_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = set_gripper_control_mode_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.set_gripper_control_mode.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_set_gripper_control_mode_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_set_gripper_control_mode_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.set_gripper_control_mode.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_set_gripper_control_mode_submit_at(fci_arm_endpoint_t *endpoint,
    const set_gripper_control_mode_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_set_gripper_control_mode_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_set_gripper_control_mode_prepare;
  observer.notify = fci_arm_endpoint_set_gripper_control_mode_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 25115U, 25116U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_set_gripper_control_mode_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_set_gripper_control_mode_async(fci_arm_endpoint_t *endpoint,
    const set_gripper_control_mode_request_value_t *request, uint32_t timeout_ms,
    fci_arm_set_gripper_control_mode_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_set_gripper_control_mode_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  set_gripper_control_mode_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const set_gripper_control_mode_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_set_gripper_control_mode_sync_state_t;

static inline void fci_arm_endpoint_set_gripper_control_mode_sync_done(void *context,
    const wl_rpc_completion_t *result, const set_gripper_control_mode_response_value_t *response) {
  fci_arm_set_gripper_control_mode_sync_state_t *state = (fci_arm_set_gripper_control_mode_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_set_gripper_control_mode_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_set_gripper_control_mode_sync_state_t *state = (fci_arm_set_gripper_control_mode_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_set_gripper_control_mode_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_set_gripper_control_mode_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_set_gripper_control_mode_sync(fci_arm_endpoint_t *endpoint,
    const set_gripper_control_mode_request_value_t *request, set_gripper_control_mode_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_set_gripper_control_mode_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_set_gripper_control_mode_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_set_gripper_control_mode_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_set_gripper_control_mode_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
/* Generator internals: all services reuse one bounded completion scratch. */
static inline void fci_arm_endpoint_set_zero_prepare(void *context,
    const wl_rpc_client_result_t *client) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  wl_rpc_completion_t *result = &endpoint->private_state.completion;
  wl_rpc_async_completion(client, result);
  if (result->status == WL_RPC_SUCCESS) {
    if (client->response_length < 20U) {
      result->status = WL_RPC_FAILED;
      result->runtime_error = WL_RPC_ERR_MALFORMED_METADATA;
      return;
    }
    result->codec_error = set_zero_response_value_decode(client->response_data + 20U,
        client->response_length - 20U, &endpoint->private_state.values.set_zero.response);
    if (result->codec_error != WL_CODEC_OK) result->status = WL_RPC_FAILED;
  }
}
static inline void fci_arm_endpoint_set_zero_notify(void *context,
    wl_rpc_callback_t callback, void *user_data) {
  fci_arm_endpoint_t *endpoint = (fci_arm_endpoint_t *)context;
  ((fci_arm_set_zero_completion_fn)callback)(user_data, &endpoint->private_state.completion,
      endpoint->private_state.completion.status == WL_RPC_SUCCESS
          ? &endpoint->private_state.values.set_zero.response : NULL);
}

/* Snapshot and accept a call; callback is delivered by step or orderly close.
 * out_call is optional cancellation authority. Successful completion owns its
 * fields; copy *response during the callback to save it. Never release a slot.
 * No callback on failed admission; BUSY means local bounded capacity is full. */
static inline wl_err_t fci_arm_endpoint_set_zero_submit_at(fci_arm_endpoint_t *endpoint,
    const set_zero_request_value_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms,
    fci_arm_set_zero_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_rpc_async_observer_t observer;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  observer.prepare = fci_arm_endpoint_set_zero_prepare;
  observer.notify = fci_arm_endpoint_set_zero_notify;
  observer.context = endpoint;
  observer.callback = (wl_rpc_callback_t)callback;
  observer.user_data = user_data;
  return wl_rpc_async_submit(&endpoint->private_state.async, 24839U, 24840U,
      WL_DELIVERY_RELIABLE, timeout_ms, now_ms, fci_arm_set_zero_encode_submission, request,
      &observer, out_call);
}

static inline wl_err_t fci_arm_endpoint_set_zero_async(fci_arm_endpoint_t *endpoint,
    const set_zero_request_value_t *request, uint32_t timeout_ms,
    fci_arm_set_zero_completion_fn callback, void *user_data, wl_rpc_call_t *out_call) {
  wl_time_ms_t now_ms;
  int error;
  if (endpoint == NULL || request == NULL || callback == NULL) return WL_ERR_INVALID_ARG;
  if (endpoint->private_state.closing) return WL_ERR_NOT_INITIALIZED;
  error = wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  if (error != WL_OK) return error;
  return fci_arm_endpoint_set_zero_submit_at(endpoint, request, timeout_ms, now_ms,
      callback, user_data, out_call);
}

/* Internal stack completion; no large response temporary or borrowed fields. */
typedef struct {
  bool done;
  wl_rpc_completion_t result;
  set_zero_response_value_t *response;
  fci_arm_endpoint_t *endpoint;
  const set_zero_request_value_t *request;
  wl_rpc_sync_notify_fn notify;
  void *notify_context;
} fci_arm_set_zero_sync_state_t;

static inline void fci_arm_endpoint_set_zero_sync_done(void *context,
    const wl_rpc_completion_t *result, const set_zero_response_value_t *response) {
  fci_arm_set_zero_sync_state_t *state = (fci_arm_set_zero_sync_state_t *)context;
  state->result = *result;
  if (response != NULL) *state->response = *response;
  state->done = true;
  if (state->notify != NULL) state->notify(state->notify_context, &state->result);
}

static inline wl_err_t fci_arm_endpoint_set_zero_proxy_submit(void *context,
    wl_time_ms_t deadline, wl_rpc_sync_notify_fn notify, void *notify_context,
    wl_rpc_call_t *call) {
  fci_arm_set_zero_sync_state_t *state = (fci_arm_set_zero_sync_state_t *)context;
  wl_time_ms_t now_ms;
  int error = wl_endpoint_now(fci_arm_endpoint_handle(state->endpoint), &now_ms);
  if (error != WL_OK) return error;
  const uint32_t remaining = deadline - now_ms;
  if (remaining == 0U || remaining > INT32_MAX) return WL_ERR_TIMEOUT;
  state->notify = notify;
  state->notify_context = notify_context;
  return fci_arm_endpoint_set_zero_submit_at(state->endpoint, state->request, remaining, now_ms,
      fci_arm_endpoint_set_zero_sync_done, state, call);
}

/* Platform call: use the installed waiter on the owner thread, or the bound
 * executor's proxy from business threads. Reuses async deadlines/completion;
 * never call from the same owner's callbacks. Response changes only on SUCCESS. Admission
 * and platform failures use local_error, not business rejection/transport_error.
 * On return there is no remaining callback referring to this function's stack. */
static inline wl_rpc_completion_t fci_arm_endpoint_set_zero_sync(fci_arm_endpoint_t *endpoint,
    const set_zero_request_value_t *request, set_zero_response_value_t *response, uint32_t timeout_ms) {
  fci_arm_set_zero_sync_state_t state;
  wl_rpc_call_t call;
  wl_waiter_t waiter;
  const wl_waiter_t *platform;
  const wl_rpc_executor_t *executor;
  wl_err_t error;
  memset(&state, 0, sizeof(state));
  state.result.status = WL_RPC_FAILED;
  state.response = response;
  state.endpoint = endpoint;
  state.request = request;
  if (endpoint == NULL || request == NULL || response == NULL) {
    state.result.local_error = WL_ERR_INVALID_ARG;
    return state.result;
  }
  /* The binding is immutable while callers run. Do not inspect mutable owner
   * state on a proxy caller thread, including during executor shutdown. */
  executor = wl_endpoint_rpc_executor(fci_arm_endpoint_handle(endpoint));
  if (executor != NULL) {
    const wl_rpc_sync_call_t proxy = {&state, fci_arm_endpoint_set_zero_proxy_submit};
    return executor->invoke(executor->context, &proxy, timeout_ms);
  }
  if (endpoint->private_state.stepping || endpoint->private_state.closing ||
      endpoint->private_state.sync_waiting) {
    state.result.local_error = WL_ERR_REENTRANT;
    return state.result;
  }
  if (wl_endpoint_link(fci_arm_endpoint_handle(endpoint)) == NULL) {
    state.result.local_error = WL_ERR_NOT_INITIALIZED;
    return state.result;
  }
  platform = wl_endpoint_waiter(fci_arm_endpoint_handle(endpoint));
  if (platform == NULL) {
    state.result.local_error = WL_ERR_NOT_SUPPORTED;
    return state.result;
  }
  waiter = *platform;
  endpoint->private_state.sync_waiting = true;
  error = fci_arm_endpoint_set_zero_async(endpoint, request, timeout_ms,
      fci_arm_endpoint_set_zero_sync_done, &state, &call);
  if (error == WL_OK) {
    while (!state.done) {
      wl_poll_hint_t hint;
      error = fci_arm_endpoint_step(endpoint);
      if (state.done || error != WL_OK) break;
      error = wl_endpoint_get_hint(fci_arm_endpoint_handle(endpoint), &hint);
      if (error != WL_OK) break;
      if (hint.work_pending || hint.next_deadline_ms == 0U) continue;
      error = waiter.wait(waiter.user_data, hint.next_deadline_ms);
      if (error == WL_ERR_NO_DATA) error = WL_OK;
      if (error != WL_OK) break;
    }
    if (!state.done) {
      /* The accepted handle is still live. Shared notification machinery
       * detaches this stack context without closing unrelated calls. */
      endpoint->private_state.stepping = true;
      (void)wl_rpc_async_cancel_complete(&endpoint->private_state.async, &call);
      endpoint->private_state.stepping = false;
      state.result.status = error == WL_ERR_CANCELLED ? WL_RPC_CANCELLED : WL_RPC_FAILED;
      state.result.local_error = error;
    }
  } else {
    state.result.local_error = error;
  }
  endpoint->private_state.sync_waiting = false;
  return state.result;
}
#ifdef __cplusplus
}
#endif

#endif
