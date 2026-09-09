#include "fci_arm_runtime.h"

#include <string.h>

static fci_arm_runtime_result_t fci_arm_runtime_result(const wl_event_t *event) {
  fci_arm_runtime_result_t result = {0};
  result.domain = FCI_ARM_RUNTIME_INVALID_ARGUMENT;
  if (event != NULL) {
    result.message_id = event->message_id;
    result.event_type = event->type;
  }
  return result;
}

const char *fci_arm_runtime_result_str(const fci_arm_runtime_result_t *result) {
  if (result == NULL) return "null result";
  switch (result->domain) {
    case FCI_ARM_RUNTIME_OK: return "ok";
    case FCI_ARM_RUNTIME_NON_RX: return "non-rx event";
    case FCI_ARM_RUNTIME_UNKNOWN_MESSAGE: return "unknown message";
    case FCI_ARM_RUNTIME_MISSING_ROUTE: return "missing route";
    case FCI_ARM_RUNTIME_MISSING_SCRATCH: return "missing scratch";
    case FCI_ARM_RUNTIME_DELIVERY_MISMATCH: return "delivery mismatch";
    case FCI_ARM_RUNTIME_CODEC_ERROR: return "codec error";
    case FCI_ARM_RUNTIME_STORAGE_ERROR: return "storage error";
    case FCI_ARM_RUNTIME_RPC_ERROR: return "rpc error";
    case FCI_ARM_RUNTIME_CORE_ERROR: return "core error";
    case FCI_ARM_RUNTIME_APPLICATION_ERROR: return "application error";
    case FCI_ARM_RUNTIME_INVALID_ARGUMENT: return "invalid argument";
    default: return "unknown runtime result";
  }
}

static const uint64_t fci_arm_rpc_fingerprint_seed = UINT64_C(0x24faaea3493c1c2e);
wl_codec_status_t acquire_control_lease_request_wlc_detail_fingerprint(const acquire_control_lease_request_t *, uint64_t *, size_t *);
#if ACQUIRE_CONTROL_LEASE_REQUEST_HAS_VALUE
void acquire_control_lease_request_wlc_detail_value_copy(const acquire_control_lease_request_t *, acquire_control_lease_request_value_t *);
#endif

wl_codec_status_t clear_error_request_wlc_detail_fingerprint(const clear_error_request_t *, uint64_t *, size_t *);
#if CLEAR_ERROR_REQUEST_HAS_VALUE
void clear_error_request_wlc_detail_value_copy(const clear_error_request_t *, clear_error_request_value_t *);
#endif

wl_codec_status_t clear_faults_request_wlc_detail_fingerprint(const clear_faults_request_t *, uint64_t *, size_t *);
#if CLEAR_FAULTS_REQUEST_HAS_VALUE
void clear_faults_request_wlc_detail_value_copy(const clear_faults_request_t *, clear_faults_request_value_t *);
#endif

wl_codec_status_t emergency_stop_request_wlc_detail_fingerprint(const emergency_stop_request_t *, uint64_t *, size_t *);
#if EMERGENCY_STOP_REQUEST_HAS_VALUE
void emergency_stop_request_wlc_detail_value_copy(const emergency_stop_request_t *, emergency_stop_request_value_t *);
#endif

wl_codec_status_t get_device_info_request_wlc_detail_fingerprint(const get_device_info_request_t *, uint64_t *, size_t *);
#if GET_DEVICE_INFO_REQUEST_HAS_VALUE
void get_device_info_request_wlc_detail_value_copy(const get_device_info_request_t *, get_device_info_request_value_t *);
#endif

wl_codec_status_t get_device_settings_request_wlc_detail_fingerprint(const get_device_settings_request_t *, uint64_t *, size_t *);
#if GET_DEVICE_SETTINGS_REQUEST_HAS_VALUE
void get_device_settings_request_wlc_detail_value_copy(const get_device_settings_request_t *, get_device_settings_request_value_t *);
#endif

wl_codec_status_t get_motor_feedback_request_wlc_detail_fingerprint(const get_motor_feedback_request_t *, uint64_t *, size_t *);
#if GET_MOTOR_FEEDBACK_REQUEST_HAS_VALUE
void get_motor_feedback_request_wlc_detail_value_copy(const get_motor_feedback_request_t *, get_motor_feedback_request_value_t *);
#endif

wl_codec_status_t home_request_wlc_detail_fingerprint(const home_request_t *, uint64_t *, size_t *);
#if HOME_REQUEST_HAS_VALUE
void home_request_wlc_detail_value_copy(const home_request_t *, home_request_value_t *);
#endif

wl_codec_status_t motor_register_read_request_wlc_detail_fingerprint(const motor_register_read_request_t *, uint64_t *, size_t *);
#if MOTOR_REGISTER_READ_REQUEST_HAS_VALUE
void motor_register_read_request_wlc_detail_value_copy(const motor_register_read_request_t *, motor_register_read_request_value_t *);
#endif

wl_codec_status_t motor_register_write_request_wlc_detail_fingerprint(const motor_register_write_request_t *, uint64_t *, size_t *);
#if MOTOR_REGISTER_WRITE_REQUEST_HAS_VALUE
void motor_register_write_request_wlc_detail_value_copy(const motor_register_write_request_t *, motor_register_write_request_value_t *);
#endif

wl_codec_status_t motor_set_zero_request_wlc_detail_fingerprint(const motor_set_zero_request_t *, uint64_t *, size_t *);
#if MOTOR_SET_ZERO_REQUEST_HAS_VALUE
void motor_set_zero_request_wlc_detail_value_copy(const motor_set_zero_request_t *, motor_set_zero_request_value_t *);
#endif

wl_codec_status_t motor_store_parameters_request_wlc_detail_fingerprint(const motor_store_parameters_request_t *, uint64_t *, size_t *);
#if MOTOR_STORE_PARAMETERS_REQUEST_HAS_VALUE
void motor_store_parameters_request_wlc_detail_value_copy(const motor_store_parameters_request_t *, motor_store_parameters_request_value_t *);
#endif

wl_codec_status_t release_control_lease_request_wlc_detail_fingerprint(const release_control_lease_request_t *, uint64_t *, size_t *);
#if RELEASE_CONTROL_LEASE_REQUEST_HAS_VALUE
void release_control_lease_request_wlc_detail_value_copy(const release_control_lease_request_t *, release_control_lease_request_value_t *);
#endif

wl_codec_status_t set_arm_control_mode_request_wlc_detail_fingerprint(const set_arm_control_mode_request_t *, uint64_t *, size_t *);
#if SET_ARM_CONTROL_MODE_REQUEST_HAS_VALUE
void set_arm_control_mode_request_wlc_detail_value_copy(const set_arm_control_mode_request_t *, set_arm_control_mode_request_value_t *);
#endif

wl_codec_status_t set_arm_mode_request_wlc_detail_fingerprint(const set_arm_mode_request_t *, uint64_t *, size_t *);
#if SET_ARM_MODE_REQUEST_HAS_VALUE
void set_arm_mode_request_wlc_detail_value_copy(const set_arm_mode_request_t *, set_arm_mode_request_value_t *);
#endif

wl_codec_status_t set_device_info_request_wlc_detail_fingerprint(const set_device_info_request_t *, uint64_t *, size_t *);
#if SET_DEVICE_INFO_REQUEST_HAS_VALUE
void set_device_info_request_wlc_detail_value_copy(const set_device_info_request_t *, set_device_info_request_value_t *);
#endif

wl_codec_status_t set_device_settings_request_wlc_detail_fingerprint(const set_device_settings_request_t *, uint64_t *, size_t *);
#if SET_DEVICE_SETTINGS_REQUEST_HAS_VALUE
void set_device_settings_request_wlc_detail_value_copy(const set_device_settings_request_t *, set_device_settings_request_value_t *);
#endif

wl_codec_status_t set_gripper_control_mode_request_wlc_detail_fingerprint(const set_gripper_control_mode_request_t *, uint64_t *, size_t *);
#if SET_GRIPPER_CONTROL_MODE_REQUEST_HAS_VALUE
void set_gripper_control_mode_request_wlc_detail_value_copy(const set_gripper_control_mode_request_t *, set_gripper_control_mode_request_value_t *);
#endif

wl_codec_status_t set_zero_request_wlc_detail_fingerprint(const set_zero_request_t *, uint64_t *, size_t *);
#if SET_ZERO_REQUEST_HAS_VALUE
void set_zero_request_wlc_detail_value_copy(const set_zero_request_t *, set_zero_request_value_t *);
#endif

static void fci_arm_runtime_cancel_peer_tx(void *context, wl_tx_handle_t handle) {
  if (context != NULL) (void)wl_tx_cancel((wl_ctx_t *)context, handle);
}

wl_err_t fci_arm_runtime_config_defaults(fci_arm_runtime_config_t *config) {
  if (config == NULL) return WL_ERR_INVALID_ARG;
  memset(config, 0, sizeof(*config));
  config->arm_status_latest_initial_generation = 1U;
  config->motor_feedback_latest_initial_generation = 1U;
  config->arm_diagnostics_latest_initial_generation = 1U;
  config->rpc_client_slot_count = 1U;
  config->rpc_client_next_operation_id = 1U;
  config->rpc_server_pending_slot_count = 1U;
  config->rpc_server_cache_slot_count = 1U;
  config->rpc_server_cache_policy = WL_RPC_CACHE_REJECT_NEW;
  config->rpc_client_response_capacity = 218U;
  config->rpc_server_response_capacity = 218U;
  return WL_OK;
}

wl_err_t fci_arm_runtime_config_enable_client(fci_arm_runtime_config_t *config) {
  if (config == NULL) return WL_ERR_INVALID_ARG;
  if (!FCI_ARM_RUNTIME_HAS_RPC_CLIENT || config->rpc_client_slot_count == 0U || config->rpc_client_response_capacity == 0U) return WL_ERR_NOT_SUPPORTED;
  config->rpc_client_enabled = 1U;
  return WL_OK;
}

wl_err_t fci_arm_runtime_config_enable_server(fci_arm_runtime_config_t *config) {
  if (config == NULL) return WL_ERR_INVALID_ARG;
  if (!FCI_ARM_RUNTIME_HAS_RPC_SERVER || config->rpc_server_pending_slot_count == 0U || config->rpc_server_cache_slot_count == 0U || config->rpc_server_response_capacity == 0U) return WL_ERR_NOT_SUPPORTED;
  config->rpc_server_enabled = 1U;
  return WL_OK;
}

fci_arm_runtime_storage_t fci_arm_runtime_default_storage_descriptor(fci_arm_runtime_default_storage_t *storage) {
  fci_arm_runtime_storage_t descriptor = {0};
  if (storage != NULL) {
    descriptor.data = storage->bytes;
    descriptor.size = sizeof(storage->bytes);
  }
  return descriptor;
}

static int fci_arm_runtime_roles_valid(const fci_arm_runtime_config_t *config) {
  if (config->rpc_server_enabled) return WL_ERR_NOT_SUPPORTED;
  if (config->acquire_control_lease_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->clear_error_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->clear_faults_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->emergency_stop_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->get_device_info_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->get_device_settings_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->get_motor_feedback_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->home_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->motor_register_read_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->motor_register_write_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->motor_set_zero_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->motor_store_parameters_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->release_control_lease_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->set_arm_control_mode_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->set_arm_mode_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->set_device_info_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->set_device_settings_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->set_gripper_control_mode_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  if (config->set_zero_request_handler != NULL) return WL_ERR_NOT_SUPPORTED;
  (void)config;
  return WL_OK;
}

const char *fci_arm_runtime_init_issue_str(fci_arm_runtime_init_issue_t issue) {
  switch (issue) {
    case FCI_ARM_RUNTIME_INIT_OK: return "ok";
    case FCI_ARM_RUNTIME_INIT_NULL_ARGUMENT: return "null argument";
    case FCI_ARM_RUNTIME_INIT_ROLE_ENABLE: return "role enable must be zero or one";
    case FCI_ARM_RUNTIME_INIT_RETAINED_CAPACITY: return "retained capacity is zero";
    case FCI_ARM_RUNTIME_INIT_RPC_CLIENT_CAPACITY: return "RPC client capacity is zero";
    case FCI_ARM_RUNTIME_INIT_RPC_SERVER_CAPACITY: return "RPC server capacity is zero";
    case FCI_ARM_RUNTIME_INIT_RPC_TIMEOUT: return "RPC timeout exceeds wrap-safe range";
    case FCI_ARM_RUNTIME_INIT_RPC_CACHE_POLICY: return "unknown RPC cache policy";
    case FCI_ARM_RUNTIME_INIT_LAYOUT_OVERFLOW: return "runtime layout size overflow";
    case FCI_ARM_RUNTIME_INIT_STORAGE_TOO_SMALL: return "runtime storage is too small";
    case FCI_ARM_RUNTIME_INIT_STORAGE_NULL: return "runtime storage data is null";
    case FCI_ARM_RUNTIME_INIT_STORAGE_ALIGNMENT: return "runtime storage is misaligned";
    case FCI_ARM_RUNTIME_INIT_STORAGE_OVERLAP: return "runtime storage overlaps the instance";
    case FCI_ARM_RUNTIME_INIT_COMPONENT: return "runtime component initialization failed";
    default: return "unknown runtime initialization issue";
  }
}

static int fci_arm_runtime_init_failure(fci_arm_runtime_init_diagnostic_t *diagnostic, fci_arm_runtime_init_issue_t issue, const char *field, size_t required, size_t provided, int result) {
  if (diagnostic != NULL) {
    diagnostic->issue = issue;
    diagnostic->field = field;
    diagnostic->required = required;
    diagnostic->provided = provided;
  }
  return result;
}

typedef struct {
  uint8_t *base;
  size_t size;
  size_t offset;
} fci_arm_runtime_storage_cursor_t;

typedef struct {
  void *arm_status_latest_storage;
  void *motor_feedback_latest_storage;
  void *arm_diagnostics_latest_storage;
  void *rpc_client_slots;
  void *rpc_client_responses;
  size_t rpc_client_responses_size;
  void *rpc_server_pending_slots;
  void *rpc_server_cache_slots;
  void *rpc_server_responses;
  size_t rpc_server_responses_size;
} fci_arm_runtime_layout_t;

static inline int fci_arm_runtime_storage_region(fci_arm_runtime_storage_cursor_t *cursor, size_t alignment, size_t count, size_t element_size, void **out_data, size_t *out_size) {
  size_t aligned;
  size_t region_size;
  if (cursor == NULL || alignment == 0U || (alignment & (alignment - 1U)) != 0U) return WL_ERR_INVALID_ARG;
  if (out_data != NULL) *out_data = NULL;
  if (out_size != NULL) *out_size = 0U;
  if (count != 0U && element_size > SIZE_MAX / count) return WL_ERR_INVALID_ARG;
  region_size = count * element_size;
  if (cursor->offset > SIZE_MAX - (alignment - 1U)) return WL_ERR_INVALID_ARG;
  aligned = (cursor->offset + (alignment - 1U)) & ~(alignment - 1U);
  if (region_size > SIZE_MAX - aligned) return WL_ERR_INVALID_ARG;
  if (aligned + region_size > cursor->size) return WL_ERR_BUF_TOO_SMALL;
  if (out_data != NULL && cursor->base != NULL) *out_data = cursor->base + aligned;
  if (out_size != NULL) *out_size = region_size;
  cursor->offset = aligned + region_size;
  return WL_OK;
}

static int fci_arm_runtime_layout(const fci_arm_runtime_config_t *config, uint8_t *base, size_t size, fci_arm_runtime_layout_t *out_layout, fci_arm_runtime_requirements_t *out_requirements) {
  fci_arm_runtime_storage_cursor_t cursor = {base, size, 0U};
  size_t alignment = 1U;
  int result;
  if (out_layout != NULL) memset(out_layout, 0, sizeof(*out_layout));
  if (out_requirements != NULL) memset(out_requirements, 0, sizeof(*out_requirements));
  if (config == NULL) return WL_ERR_INVALID_ARG;
  {
    const wl_latest_config_t route_config = {sizeof(arm_status_t), _Alignof(arm_status_t), config->arm_status_latest_initial_generation};
    wl_latest_requirements_t route_requirements;
    result = wl_latest_requirements(&route_config, &route_requirements);
    if (result != WL_OK) return result;
    if (alignment < _Alignof(arm_status_t)) alignment = _Alignof(arm_status_t);
    result = fci_arm_runtime_storage_region(&cursor, _Alignof(arm_status_t), 1U, route_requirements.storage_size, out_layout == NULL ? NULL : &out_layout->arm_status_latest_storage, NULL);
    if (result != WL_OK) return result;
  }
  {
    const wl_latest_config_t route_config = {sizeof(motor_feedback_t), _Alignof(motor_feedback_t), config->motor_feedback_latest_initial_generation};
    wl_latest_requirements_t route_requirements;
    result = wl_latest_requirements(&route_config, &route_requirements);
    if (result != WL_OK) return result;
    if (alignment < _Alignof(motor_feedback_t)) alignment = _Alignof(motor_feedback_t);
    result = fci_arm_runtime_storage_region(&cursor, _Alignof(motor_feedback_t), 1U, route_requirements.storage_size, out_layout == NULL ? NULL : &out_layout->motor_feedback_latest_storage, NULL);
    if (result != WL_OK) return result;
  }
  {
    const wl_latest_config_t route_config = {sizeof(arm_diagnostics_t), _Alignof(arm_diagnostics_t), config->arm_diagnostics_latest_initial_generation};
    wl_latest_requirements_t route_requirements;
    result = wl_latest_requirements(&route_config, &route_requirements);
    if (result != WL_OK) return result;
    if (alignment < _Alignof(arm_diagnostics_t)) alignment = _Alignof(arm_diagnostics_t);
    result = fci_arm_runtime_storage_region(&cursor, _Alignof(arm_diagnostics_t), 1U, route_requirements.storage_size, out_layout == NULL ? NULL : &out_layout->arm_diagnostics_latest_storage, NULL);
    if (result != WL_OK) return result;
  }
  if (fci_arm_runtime_roles_valid(config) != WL_OK) return WL_ERR_NOT_SUPPORTED;
  if (config->rpc_client_enabled > 1U || config->rpc_server_enabled > 1U) return WL_ERR_INVALID_ARG;
  if (config->rpc_client_enabled != 0U) {
    if (config->rpc_client_slot_count == 0U || config->rpc_client_response_capacity == 0U) return WL_ERR_INVALID_ARG;
    if (alignment < _Alignof(wl_rpc_client_slot_t)) alignment = _Alignof(wl_rpc_client_slot_t);
    result = fci_arm_runtime_storage_region(&cursor, _Alignof(wl_rpc_client_slot_t), config->rpc_client_slot_count, sizeof(wl_rpc_client_slot_t), out_layout == NULL ? NULL : &out_layout->rpc_client_slots, NULL);
    if (result != WL_OK) return result;
    result = fci_arm_runtime_storage_region(&cursor, 1U, config->rpc_client_slot_count, config->rpc_client_response_capacity, out_layout == NULL ? NULL : &out_layout->rpc_client_responses, out_layout == NULL ? NULL : &out_layout->rpc_client_responses_size);
    if (result != WL_OK) return result;
  }
  if (config->rpc_server_enabled != 0U) {
    if (config->rpc_server_pending_slot_count == 0U || config->rpc_server_cache_slot_count == 0U || config->rpc_server_response_capacity == 0U) return WL_ERR_INVALID_ARG;
    if ((config->rpc_server_pending_timeout_ms != 0U && config->rpc_server_pending_timeout_ms >= UINT32_C(0x80000000)) || (config->rpc_server_cache_ttl_ms != 0U && config->rpc_server_cache_ttl_ms >= UINT32_C(0x80000000))) return WL_ERR_INVALID_ARG;
    if (config->rpc_server_cache_policy != WL_RPC_CACHE_REJECT_NEW && config->rpc_server_cache_policy != WL_RPC_CACHE_EVICT_OLDEST) return WL_ERR_INVALID_ARG;
    if (alignment < _Alignof(wl_rpc_server_pending_slot_t)) alignment = _Alignof(wl_rpc_server_pending_slot_t);
    if (alignment < _Alignof(wl_rpc_server_cache_slot_t)) alignment = _Alignof(wl_rpc_server_cache_slot_t);
    result = fci_arm_runtime_storage_region(&cursor, _Alignof(wl_rpc_server_pending_slot_t), config->rpc_server_pending_slot_count, sizeof(wl_rpc_server_pending_slot_t), out_layout == NULL ? NULL : &out_layout->rpc_server_pending_slots, NULL);
    if (result != WL_OK) return result;
    result = fci_arm_runtime_storage_region(&cursor, _Alignof(wl_rpc_server_cache_slot_t), config->rpc_server_cache_slot_count, sizeof(wl_rpc_server_cache_slot_t), out_layout == NULL ? NULL : &out_layout->rpc_server_cache_slots, NULL);
    if (result != WL_OK) return result;
    result = fci_arm_runtime_storage_region(&cursor, 1U, config->rpc_server_cache_slot_count, config->rpc_server_response_capacity, out_layout == NULL ? NULL : &out_layout->rpc_server_responses, out_layout == NULL ? NULL : &out_layout->rpc_server_responses_size);
    if (result != WL_OK) return result;
  }
  if (out_requirements != NULL) {
    out_requirements->storage_size = cursor.offset;
    out_requirements->storage_alignment = alignment;
  }
  return WL_OK;
}

int fci_arm_runtime_requirements(const fci_arm_runtime_config_t *config, fci_arm_runtime_requirements_t *out_requirements) {
  fci_arm_runtime_config_t config_copy;
  if (config == NULL || out_requirements == NULL) return WL_ERR_INVALID_ARG;
  config_copy = *config;
  *out_requirements = (fci_arm_runtime_requirements_t){0};
  return fci_arm_runtime_layout(&config_copy, NULL, SIZE_MAX, NULL, out_requirements);
}

static int fci_arm_runtime_init_validate(const fci_arm_runtime_instance_t *instance, const fci_arm_runtime_config_t *config, const fci_arm_runtime_storage_t *storage, fci_arm_runtime_requirements_t *requirements, fci_arm_runtime_init_diagnostic_t *diagnostic) {
  uintptr_t instance_address;
  uintptr_t storage_address;
  int result;
  if (diagnostic != NULL) memset(diagnostic, 0, sizeof(*diagnostic));
  if (instance == NULL || config == NULL || storage == NULL || requirements == NULL || diagnostic == NULL)
    return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_NULL_ARGUMENT, instance == NULL ? "instance" : config == NULL ? "config" : storage == NULL ? "storage" : requirements == NULL ? "requirements" : "diagnostic", 1U, 0U, WL_ERR_INVALID_ARG);
  if (fci_arm_runtime_roles_valid(config) != WL_OK) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_ROLE_ENABLE, "endpoint.rpc_role", 0U, 1U, WL_ERR_NOT_SUPPORTED);
  if (config->rpc_client_enabled > 1U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_ROLE_ENABLE, "rpc_client_enabled", 1U, config->rpc_client_enabled, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled > 1U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_ROLE_ENABLE, "rpc_server_enabled", 1U, config->rpc_server_enabled, WL_ERR_INVALID_ARG);
  if (config->rpc_client_enabled != 0U && config->rpc_client_slot_count == 0U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_CLIENT_CAPACITY, "rpc_client_slot_count", 1U, 0U, WL_ERR_INVALID_ARG);
  if (config->rpc_client_enabled != 0U && config->rpc_client_response_capacity == 0U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_CLIENT_CAPACITY, "rpc_client_response_capacity", 1U, 0U, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled != 0U && config->rpc_server_pending_slot_count == 0U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_SERVER_CAPACITY, "rpc_server_pending_slot_count", 1U, 0U, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled != 0U && config->rpc_server_cache_slot_count == 0U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_SERVER_CAPACITY, "rpc_server_cache_slot_count", 1U, 0U, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled != 0U && config->rpc_server_response_capacity == 0U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_SERVER_CAPACITY, "rpc_server_response_capacity", 1U, 0U, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled != 0U && config->rpc_server_pending_timeout_ms >= UINT32_C(0x80000000)) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_TIMEOUT, "rpc_server_pending_timeout_ms", UINT32_C(0x7fffffff), config->rpc_server_pending_timeout_ms, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled != 0U && config->rpc_server_cache_ttl_ms >= UINT32_C(0x80000000)) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_TIMEOUT, "rpc_server_cache_ttl_ms", UINT32_C(0x7fffffff), config->rpc_server_cache_ttl_ms, WL_ERR_INVALID_ARG);
  if (config->rpc_server_enabled != 0U && config->rpc_server_cache_policy != WL_RPC_CACHE_REJECT_NEW && config->rpc_server_cache_policy != WL_RPC_CACHE_EVICT_OLDEST) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_RPC_CACHE_POLICY, "rpc_server_cache_policy", 0U, (size_t)config->rpc_server_cache_policy, WL_ERR_INVALID_ARG);
  result = fci_arm_runtime_requirements(config, requirements);
  if (result != WL_OK) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_LAYOUT_OVERFLOW, "config", 0U, 0U, result);
  if (storage->size < requirements->storage_size) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_STORAGE_TOO_SMALL, "storage.size", requirements->storage_size, storage->size, WL_ERR_BUF_TOO_SMALL);
  if (requirements->storage_size != 0U) {
    if (storage->data == NULL) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_STORAGE_NULL, "storage.data", requirements->storage_size, 0U, WL_ERR_INVALID_ARG);
    if (((uintptr_t)storage->data & (requirements->storage_alignment - 1U)) != 0U) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_STORAGE_ALIGNMENT, "storage.data", requirements->storage_alignment, (size_t)((uintptr_t)storage->data & (requirements->storage_alignment - 1U)), WL_ERR_INVALID_ARG);
    instance_address = (uintptr_t)(const void *)instance;
    storage_address = (uintptr_t)storage->data;
    if ((storage_address <= instance_address && instance_address - storage_address < requirements->storage_size) || (instance_address < storage_address && storage_address - instance_address < sizeof(*instance))) return fci_arm_runtime_init_failure(diagnostic, FCI_ARM_RUNTIME_INIT_STORAGE_OVERLAP, "storage.data", requirements->storage_size, storage->size, WL_ERR_INVALID_ARG);
  }
  return WL_OK;
}

int fci_arm_runtime_init(fci_arm_runtime_instance_t *instance, const fci_arm_runtime_config_t *config, const fci_arm_runtime_storage_t *storage) {
  fci_arm_runtime_config_t config_copy;
  fci_arm_runtime_storage_t storage_copy;
  fci_arm_runtime_requirements_t requirements;
  fci_arm_runtime_layout_t layout;
  uintptr_t instance_address;
  uintptr_t storage_address;
  int result;
  if (instance == NULL || config == NULL || storage == NULL) return WL_ERR_INVALID_ARG;
  config_copy = *config;
  storage_copy = *storage;
  config = &config_copy;
  storage = &storage_copy;
  result = fci_arm_runtime_requirements(config, &requirements);
  if (result != WL_OK) return result;
  if (storage->size < requirements.storage_size) return WL_ERR_BUF_TOO_SMALL;
  if (requirements.storage_size != 0U) {
    if (storage->data == NULL || ((uintptr_t)storage->data & (requirements.storage_alignment - 1U)) != 0U) return WL_ERR_INVALID_ARG;
    instance_address = (uintptr_t)(void *)instance;
    storage_address = (uintptr_t)storage->data;
    if ((storage_address <= instance_address && instance_address - storage_address < requirements.storage_size) || (instance_address < storage_address && storage_address - instance_address < sizeof(*instance))) return WL_ERR_INVALID_ARG;
  }
  result = fci_arm_runtime_layout(config, (uint8_t *)storage->data, storage->size, &layout, NULL);
  if (result != WL_OK) return result;
  memset(instance, 0, sizeof(*instance));
  {
    const wl_latest_config_t route_config = {sizeof(arm_status_t), _Alignof(arm_status_t), config->arm_status_latest_initial_generation};
    wl_latest_requirements_t route_requirements;
    wl_latest_storage_t route_storage;
    result = wl_latest_requirements(&route_config, &route_requirements);
    if (result != WL_OK) goto init_failed;
    route_storage.data = layout.arm_status_latest_storage;
    route_storage.size = route_requirements.storage_size;
    result = wl_latest_init(&instance->arm_status_latest, &route_config, &route_storage);
    if (result != WL_OK) goto init_failed;
    instance->runtime.arm_status_latest = &instance->arm_status_latest;
  }
  {
    const wl_latest_config_t route_config = {sizeof(motor_feedback_t), _Alignof(motor_feedback_t), config->motor_feedback_latest_initial_generation};
    wl_latest_requirements_t route_requirements;
    wl_latest_storage_t route_storage;
    result = wl_latest_requirements(&route_config, &route_requirements);
    if (result != WL_OK) goto init_failed;
    route_storage.data = layout.motor_feedback_latest_storage;
    route_storage.size = route_requirements.storage_size;
    result = wl_latest_init(&instance->motor_feedback_latest, &route_config, &route_storage);
    if (result != WL_OK) goto init_failed;
    instance->runtime.motor_feedback_latest = &instance->motor_feedback_latest;
  }
  {
    const wl_latest_config_t route_config = {sizeof(arm_diagnostics_t), _Alignof(arm_diagnostics_t), config->arm_diagnostics_latest_initial_generation};
    wl_latest_requirements_t route_requirements;
    wl_latest_storage_t route_storage;
    result = wl_latest_requirements(&route_config, &route_requirements);
    if (result != WL_OK) goto init_failed;
    route_storage.data = layout.arm_diagnostics_latest_storage;
    route_storage.size = route_requirements.storage_size;
    result = wl_latest_init(&instance->arm_diagnostics_latest, &route_config, &route_storage);
    if (result != WL_OK) goto init_failed;
    instance->runtime.arm_diagnostics_latest = &instance->arm_diagnostics_latest;
  }
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) {
    const wl_rpc_client_config_t client_config = {
      (wl_rpc_client_slot_t *)layout.rpc_client_slots,
      config->rpc_client_slot_count,
      (uint8_t *)layout.rpc_client_responses,
      layout.rpc_client_responses_size,
      config->rpc_client_response_capacity,
      config->rpc_client_next_operation_id
    };
    if (wl_rpc_client_init(&instance->rpc_client, &client_config) != WL_RPC_OK) {
      result = WL_ERR_INVALID_ARG;
      goto init_failed;
    }
    instance->runtime.rpc_client = &instance->rpc_client;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    const wl_rpc_server_config_t server_config = {
      (wl_rpc_server_pending_slot_t *)layout.rpc_server_pending_slots,
      config->rpc_server_pending_slot_count,
      (wl_rpc_server_cache_slot_t *)layout.rpc_server_cache_slots,
      config->rpc_server_cache_slot_count,
      (uint8_t *)layout.rpc_server_responses,
      layout.rpc_server_responses_size,
      config->rpc_server_response_capacity,
      config->rpc_server_pending_timeout_ms,
      config->rpc_server_cache_ttl_ms,
      config->rpc_server_cache_policy
    };
    if (wl_rpc_server_init(&instance->rpc_server, &server_config) != WL_RPC_OK) {
      result = WL_ERR_INVALID_ARG;
      goto init_failed;
    }
    instance->runtime.rpc_server = &instance->rpc_server;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.acquire_control_lease.request_scratch = &instance->acquire_control_lease_scratch.request;
    instance->runtime.acquire_control_lease.request_handler = config->acquire_control_lease_request_handler;
    instance->runtime.acquire_control_lease.user_data = config->acquire_control_lease_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.acquire_control_lease.response_scratch = &instance->acquire_control_lease_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.clear_error.request_scratch = &instance->clear_error_scratch.request;
    instance->runtime.clear_error.request_handler = config->clear_error_request_handler;
    instance->runtime.clear_error.user_data = config->clear_error_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.clear_error.response_scratch = &instance->clear_error_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.clear_faults.request_scratch = &instance->clear_faults_scratch.request;
    instance->runtime.clear_faults.request_handler = config->clear_faults_request_handler;
    instance->runtime.clear_faults.user_data = config->clear_faults_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.clear_faults.response_scratch = &instance->clear_faults_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.emergency_stop.request_scratch = &instance->emergency_stop_scratch.request;
    instance->runtime.emergency_stop.request_handler = config->emergency_stop_request_handler;
    instance->runtime.emergency_stop.user_data = config->emergency_stop_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.emergency_stop.response_scratch = &instance->emergency_stop_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.get_device_info.request_scratch = &instance->get_device_info_scratch.request;
    instance->runtime.get_device_info.request_handler = config->get_device_info_request_handler;
    instance->runtime.get_device_info.user_data = config->get_device_info_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.get_device_info.response_scratch = &instance->get_device_info_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.get_device_settings.request_scratch = &instance->get_device_settings_scratch.request;
    instance->runtime.get_device_settings.request_handler = config->get_device_settings_request_handler;
    instance->runtime.get_device_settings.user_data = config->get_device_settings_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.get_device_settings.response_scratch = &instance->get_device_settings_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.get_motor_feedback.request_scratch = &instance->get_motor_feedback_scratch.request;
    instance->runtime.get_motor_feedback.request_handler = config->get_motor_feedback_request_handler;
    instance->runtime.get_motor_feedback.user_data = config->get_motor_feedback_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.get_motor_feedback.response_scratch = &instance->get_motor_feedback_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.home.request_scratch = &instance->home_scratch.request;
    instance->runtime.home.request_handler = config->home_request_handler;
    instance->runtime.home.user_data = config->home_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.home.response_scratch = &instance->home_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.motor_register_read.request_scratch = &instance->motor_register_read_scratch.request;
    instance->runtime.motor_register_read.request_handler = config->motor_register_read_request_handler;
    instance->runtime.motor_register_read.user_data = config->motor_register_read_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.motor_register_read.response_scratch = &instance->motor_register_read_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.motor_register_write.request_scratch = &instance->motor_register_write_scratch.request;
    instance->runtime.motor_register_write.request_handler = config->motor_register_write_request_handler;
    instance->runtime.motor_register_write.user_data = config->motor_register_write_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.motor_register_write.response_scratch = &instance->motor_register_write_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.motor_set_zero.request_scratch = &instance->motor_set_zero_scratch.request;
    instance->runtime.motor_set_zero.request_handler = config->motor_set_zero_request_handler;
    instance->runtime.motor_set_zero.user_data = config->motor_set_zero_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.motor_set_zero.response_scratch = &instance->motor_set_zero_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.motor_store_parameters.request_scratch = &instance->motor_store_parameters_scratch.request;
    instance->runtime.motor_store_parameters.request_handler = config->motor_store_parameters_request_handler;
    instance->runtime.motor_store_parameters.user_data = config->motor_store_parameters_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.motor_store_parameters.response_scratch = &instance->motor_store_parameters_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.release_control_lease.request_scratch = &instance->release_control_lease_scratch.request;
    instance->runtime.release_control_lease.request_handler = config->release_control_lease_request_handler;
    instance->runtime.release_control_lease.user_data = config->release_control_lease_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.release_control_lease.response_scratch = &instance->release_control_lease_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.set_arm_control_mode.request_scratch = &instance->set_arm_control_mode_scratch.request;
    instance->runtime.set_arm_control_mode.request_handler = config->set_arm_control_mode_request_handler;
    instance->runtime.set_arm_control_mode.user_data = config->set_arm_control_mode_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.set_arm_control_mode.response_scratch = &instance->set_arm_control_mode_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.set_arm_mode.request_scratch = &instance->set_arm_mode_scratch.request;
    instance->runtime.set_arm_mode.request_handler = config->set_arm_mode_request_handler;
    instance->runtime.set_arm_mode.user_data = config->set_arm_mode_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.set_arm_mode.response_scratch = &instance->set_arm_mode_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.set_device_info.request_scratch = &instance->set_device_info_scratch.request;
    instance->runtime.set_device_info.request_handler = config->set_device_info_request_handler;
    instance->runtime.set_device_info.user_data = config->set_device_info_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.set_device_info.response_scratch = &instance->set_device_info_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.set_device_settings.request_scratch = &instance->set_device_settings_scratch.request;
    instance->runtime.set_device_settings.request_handler = config->set_device_settings_request_handler;
    instance->runtime.set_device_settings.user_data = config->set_device_settings_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.set_device_settings.response_scratch = &instance->set_device_settings_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.set_gripper_control_mode.request_scratch = &instance->set_gripper_control_mode_scratch.request;
    instance->runtime.set_gripper_control_mode.request_handler = config->set_gripper_control_mode_request_handler;
    instance->runtime.set_gripper_control_mode.user_data = config->set_gripper_control_mode_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.set_gripper_control_mode.response_scratch = &instance->set_gripper_control_mode_scratch.response;
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
  if (config->rpc_server_enabled != 0U) {
    instance->runtime.set_zero.request_scratch = &instance->set_zero_scratch.request;
    instance->runtime.set_zero.request_handler = config->set_zero_request_handler;
    instance->runtime.set_zero.user_data = config->set_zero_user_data;
  }
#endif
#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
  if (config->rpc_client_enabled != 0U) instance->runtime.set_zero.response_scratch = &instance->set_zero_scratch.response;
#endif
  if (config->rpc_client_enabled != 0U || config->rpc_server_enabled != 0U) instance->runtime.rpc_encode_scratch = &instance->rpc_encode_scratch;
  return WL_OK;

init_failed:
  memset(instance, 0, sizeof(*instance));
  return result;
}

int fci_arm_runtime_init_checked(fci_arm_runtime_instance_t *instance, const fci_arm_runtime_config_t *config, const fci_arm_runtime_storage_t *storage, fci_arm_runtime_init_diagnostic_t *out_diagnostic) {
  fci_arm_runtime_requirements_t requirements;
  int result = fci_arm_runtime_init_validate(instance, config, storage, &requirements, out_diagnostic);
  if (result != WL_OK) return result;
  result = fci_arm_runtime_init(instance, config, storage);
  if (result != WL_OK) return fci_arm_runtime_init_failure(out_diagnostic, FCI_ARM_RUNTIME_INIT_COMPONENT, "component", 0U, 0U, result);
  return WL_OK;
}

wl_rpc_err_t fci_arm_runtime_peer_observe(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, uint64_t peer_session_id, wl_rpc_peer_observation_t *out_observation) {
  wl_rpc_err_t result;
  if (out_observation != NULL) memset(out_observation, 0, sizeof(*out_observation));
  if (ctx == NULL || runtime == NULL || runtime->rpc_server == NULL || peer_session_id == 0U || out_observation == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_peer_observe(runtime->rpc_server, &runtime->rpc_peer, peer_session_id, fci_arm_runtime_cancel_peer_tx, ctx, out_observation);
  if (result == WL_RPC_OK && out_observation->changed != 0U) runtime->rpc_peer_observation = *out_observation;
  return result;
}

wl_rpc_err_t fci_arm_runtime_peer_observation_take(fci_arm_runtime_t *runtime, wl_rpc_peer_observation_t *out_observation) {
  if (out_observation != NULL) memset(out_observation, 0, sizeof(*out_observation));
  if (runtime == NULL || out_observation == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime->rpc_peer_observation.changed == 0U) return WL_RPC_ERR_NOT_FOUND;
  *out_observation = runtime->rpc_peer_observation;
  memset(&runtime->rpc_peer_observation, 0, sizeof(runtime->rpc_peer_observation));
  return WL_RPC_OK;
}

wl_rpc_err_t fci_arm_runtime_poll(fci_arm_runtime_t *runtime, wl_time_ms_t now_ms, fci_arm_runtime_poll_result_t *out_result) {
  wl_rpc_err_t result;
  wl_rpc_server_expiry_t server_expiry = {0};
  if (out_result != NULL) memset(out_result, 0, sizeof(*out_result));
  if (runtime == NULL || out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime->rpc_client != NULL) {
    result = wl_rpc_client_poll(runtime->rpc_client, now_ms, &out_result->client_timed_out);
    if (result != WL_RPC_OK) return result;
  }
  if (runtime->rpc_server != NULL) {
    result = wl_rpc_server_expired_acquire(runtime->rpc_server, now_ms, &out_result->server_expired_request);
    if (result == WL_RPC_OK) out_result->server_pending_expired = 1U;
    else if (result != WL_RPC_ERR_NOT_FOUND) return result;
    result = wl_rpc_server_poll(runtime->rpc_server, now_ms, &server_expiry);
    if (result != WL_RPC_OK) return result;
    out_result->server_cache_expired = server_expiry.cache_expired;
  }
  return WL_RPC_OK;
}

wl_rpc_err_t fci_arm_runtime_service(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, wl_time_ms_t now_ms, fci_arm_runtime_service_result_t *out_result) {
  wl_rpc_server_response_t response = {0};
  wl_rpc_err_t result;
  uint8_t reliable_response = 0U;
  if (out_result != NULL) memset(out_result, 0, sizeof(*out_result));
  if (ctx == NULL || runtime == NULL || out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  out_result->response = fci_arm_runtime_result(NULL);
  result = fci_arm_runtime_poll(runtime, now_ms, &out_result->deadlines);
  if (result != WL_RPC_OK) return result;
  if (runtime->rpc_server == NULL) return WL_RPC_OK;
  result = wl_rpc_server_response_acquire(runtime->rpc_server, &response);
  if (result == WL_RPC_ERR_NOT_FOUND) return WL_RPC_OK;
  if (result != WL_RPC_OK) return result;
  out_result->response.message_id = response.identity.response_message_id;
  out_result->response.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  out_result->response.detail.rpc.operation_id = response.identity.operation_id;
  out_result->response.detail.rpc.application_result = response.application_status;
  out_result->response.detail.rpc.payload_length = response.response_length;
  out_result->response.detail.rpc.server_response = response;
  switch (response.identity.response_message_id) {
    case ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case CLEAR_ERROR_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != CLEAR_ERROR_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case CLEAR_FAULTS_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != CLEAR_FAULTS_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case EMERGENCY_STOP_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != EMERGENCY_STOP_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case GET_DEVICE_INFO_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != GET_DEVICE_INFO_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case HOME_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != HOME_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != MOTOR_SET_ZERO_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case SET_ARM_MODE_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != SET_ARM_MODE_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case SET_DEVICE_INFO_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != SET_DEVICE_INFO_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    case SET_ZERO_RESPONSE_MESSAGE_ID:
      if (response.identity.request_message_id != SET_ZERO_REQUEST_MESSAGE_ID) {
        result = WL_RPC_ERR_RESPONSE_MISMATCH;
        break;
      }
      reliable_response = 1U;
      break;
    default:
      result = WL_RPC_ERR_RESPONSE_MISMATCH;
      break;
  }
  if (result != WL_RPC_OK) {
    (void)wl_rpc_server_response_defer(runtime->rpc_server, &response);
    return result;
  }
  if (reliable_response != 0U) {
    out_result->response.detail.rpc.core_result = wl_send_reliable(ctx, response.identity.response_message_id, response.response_data, response.response_length, now_ms, &out_result->response.detail.rpc.handle);
  } else {
    out_result->response.detail.rpc.core_result = wl_send_unreliable(ctx, response.identity.response_message_id, response.response_data, response.response_length);
  }
  if (out_result->response.detail.rpc.core_result != WL_OK) {
    result = wl_rpc_server_response_defer(runtime->rpc_server, &response);
    if (result != WL_RPC_OK) return result;
    out_result->response.domain = FCI_ARM_RUNTIME_CORE_ERROR;
    out_result->responses_deferred = 1U;
    return WL_RPC_OK;
  }
  if (reliable_response != 0U) {
    result = wl_rpc_server_response_submitted(runtime->rpc_server, &response, out_result->response.detail.rpc.handle);
  } else {
    result = wl_rpc_server_response_sent(runtime->rpc_server, &response);
  }
  if (result != WL_RPC_OK) {
    (void)wl_rpc_server_response_defer(runtime->rpc_server, &response);
    return result;
  }
  out_result->response.domain = FCI_ARM_RUNTIME_OK;
  out_result->responses_submitted = 1U;
  return WL_RPC_OK;
}

wl_rpc_err_t fci_arm_runtime_get_deadline_hint(const fci_arm_runtime_t *runtime, wl_time_ms_t now_ms, wl_rpc_deadline_hint_t *out_hint) {
  wl_rpc_deadline_hint_t component = {WL_RPC_NO_DEADLINE_MS};
  wl_rpc_err_t result;
  uint32_t nearest = WL_RPC_NO_DEADLINE_MS;
  if (out_hint != NULL) out_hint->next_deadline_ms = WL_RPC_NO_DEADLINE_MS;
  if (runtime == NULL || out_hint == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime->rpc_client != NULL) {
    result = wl_rpc_client_get_deadline_hint(runtime->rpc_client, now_ms, &component);
    if (result != WL_RPC_OK) return result;
    if (component.next_deadline_ms < nearest) nearest = component.next_deadline_ms;
  }
  if (runtime->rpc_server != NULL) {
    result = wl_rpc_server_get_deadline_hint(runtime->rpc_server, now_ms, &component);
    if (result != WL_RPC_OK) return result;
    if (component.next_deadline_ms < nearest) nearest = component.next_deadline_ms;
  }
  out_hint->next_deadline_ms = nearest;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_runtime_dispatch_event(wl_ctx_t *ctx, const wl_event_t *event, fci_arm_runtime_t *runtime, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(event);
  if (event == NULL) return result;
  (void)now_ms;
  if (event->type == WL_EVT_TX_SUCCESS || event->type == WL_EVT_TX_TIMEOUT || event->type == WL_EVT_TX_FAILED) {
    wl_tx_result_t tx_result = {0};
    if (runtime == NULL || ctx == NULL) {
      result.domain = FCI_ARM_RUNTIME_NON_RX;
      return result;
    }
    result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
    result.detail.rpc.handle = event->handle;
#if FCI_ARM_RUNTIME_HAS_MANAGED_RPC
    if (runtime->rpc_async != NULL && wl_rpc_async_retire_tx(runtime->rpc_async, event->handle)) {
      result.detail.rpc.core_result = wl_tx_take(ctx, event->handle, &tx_result);
      result.event_consumed = result.detail.rpc.core_result == WL_OK ? 1U : 0U;
      result.domain = result.detail.rpc.core_result == WL_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_CORE_ERROR;
      return result;
    }
#endif
    if (runtime->rpc_server != NULL) {
      result.detail.rpc.rpc_result = wl_rpc_server_on_tx_event(runtime->rpc_server, event);
      if (result.detail.rpc.rpc_result == WL_RPC_OK) {
        result.detail.rpc.core_result = wl_tx_take(ctx, event->handle, &tx_result);
        result.event_consumed = result.detail.rpc.core_result == WL_OK ? 1U : 0U;
        result.domain = result.detail.rpc.core_result == WL_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_CORE_ERROR;
        return result;
      }
      if (result.detail.rpc.rpc_result != WL_RPC_ERR_NOT_FOUND) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        return result;
      }
    }
    if (runtime->rpc_client != NULL) {
      result.detail.rpc.rpc_result = wl_rpc_client_on_tx_event(runtime->rpc_client, event);
      if (result.detail.rpc.rpc_result == WL_RPC_OK) {
        result.detail.rpc.core_result = wl_tx_take(ctx, event->handle, &tx_result);
        result.event_consumed = result.detail.rpc.core_result == WL_OK ? 1U : 0U;
        result.domain = result.detail.rpc.core_result == WL_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_CORE_ERROR;
      } else if (result.detail.rpc.rpc_result == WL_RPC_ERR_NOT_FOUND) result.domain = FCI_ARM_RUNTIME_NON_RX;
      else result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    } else {
      result.domain = FCI_ARM_RUNTIME_NON_RX;
    }
    return result;
  }
  if (event->type != WL_EVT_UNRELIABLE_RX && event->type != WL_EVT_RELIABLE_RX) {
    result.domain = FCI_ARM_RUNTIME_NON_RX;
    return result;
  }
  if (ctx == NULL) return result;
  if (runtime == NULL) goto release_event;

  switch (event->message_id) {
    case ARM_STATUS_MESSAGE_ID: {
      wl_latest_write_claim_t claim = {0};
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RETAINED;
      if (event->type != WL_EVT_UNRELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->arm_status_latest == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      result.detail.retained.storage_result = wl_latest_write_claim(runtime->arm_status_latest, &claim);
      if (result.detail.retained.storage_result != WL_OK) {
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      if (claim.value_size < sizeof(arm_status_t)) {
        result.detail.retained.storage_result = WL_ERR_BUF_TOO_SMALL;
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_status_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      if (((uintptr_t)claim.value % _Alignof(arm_status_t)) != 0U) {
        result.detail.retained.storage_result = WL_ERR_INVALID_ARG;
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_status_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      result.detail.retained.codec_status = arm_status_decode(event->payload, event->payload_len, (arm_status_t *)claim.value);
      if (result.detail.retained.codec_status != WL_CODEC_OK) {
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_status_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.retained.storage_result = wl_latest_write_publish(runtime->arm_status_latest, &claim);
      if (result.detail.retained.storage_result != WL_OK) {
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_status_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      result.domain = FCI_ARM_RUNTIME_OK;
      break;
    }
    case MOTOR_FEEDBACK_MESSAGE_ID: {
      wl_latest_write_claim_t claim = {0};
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RETAINED;
      if (event->type != WL_EVT_UNRELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->motor_feedback_latest == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      result.detail.retained.storage_result = wl_latest_write_claim(runtime->motor_feedback_latest, &claim);
      if (result.detail.retained.storage_result != WL_OK) {
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      if (claim.value_size < sizeof(motor_feedback_t)) {
        result.detail.retained.storage_result = WL_ERR_BUF_TOO_SMALL;
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->motor_feedback_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      if (((uintptr_t)claim.value % _Alignof(motor_feedback_t)) != 0U) {
        result.detail.retained.storage_result = WL_ERR_INVALID_ARG;
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->motor_feedback_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      result.detail.retained.codec_status = motor_feedback_decode(event->payload, event->payload_len, (motor_feedback_t *)claim.value);
      if (result.detail.retained.codec_status != WL_CODEC_OK) {
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->motor_feedback_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.retained.storage_result = wl_latest_write_publish(runtime->motor_feedback_latest, &claim);
      if (result.detail.retained.storage_result != WL_OK) {
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->motor_feedback_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      result.domain = FCI_ARM_RUNTIME_OK;
      break;
    }
    case ARM_DIAGNOSTICS_MESSAGE_ID: {
      wl_latest_write_claim_t claim = {0};
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RETAINED;
      if (event->type != WL_EVT_UNRELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->arm_diagnostics_latest == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      result.detail.retained.storage_result = wl_latest_write_claim(runtime->arm_diagnostics_latest, &claim);
      if (result.detail.retained.storage_result != WL_OK) {
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      if (claim.value_size < sizeof(arm_diagnostics_t)) {
        result.detail.retained.storage_result = WL_ERR_BUF_TOO_SMALL;
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_diagnostics_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      if (((uintptr_t)claim.value % _Alignof(arm_diagnostics_t)) != 0U) {
        result.detail.retained.storage_result = WL_ERR_INVALID_ARG;
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_diagnostics_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      result.detail.retained.codec_status = arm_diagnostics_decode(event->payload, event->payload_len, (arm_diagnostics_t *)claim.value);
      if (result.detail.retained.codec_status != WL_CODEC_OK) {
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_diagnostics_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.retained.storage_result = wl_latest_write_publish(runtime->arm_diagnostics_latest, &claim);
      if (result.detail.retained.storage_result != WL_OK) {
        result.detail.retained.abort_result = wl_latest_write_abort(runtime->arm_diagnostics_latest, &claim);
        result.domain = FCI_ARM_RUNTIME_STORAGE_ERROR;
        break;
      }
      result.domain = FCI_ARM_RUNTIME_OK;
      break;
    }
    case ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->acquire_control_lease.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = acquire_control_lease_request_decode(event->payload, event->payload_len, runtime->acquire_control_lease.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->acquire_control_lease.request_scratch->has_operation_id || runtime->acquire_control_lease.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->acquire_control_lease.request_scratch->operation_id;
      result.detail.rpc.codec_status = acquire_control_lease_request_wlc_detail_fingerprint(runtime->acquire_control_lease.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID;
      identity.response_message_id = ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->acquire_control_lease.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->acquire_control_lease.request_handler(runtime->acquire_control_lease.user_data, runtime->acquire_control_lease.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->acquire_control_lease.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = acquire_control_lease_response_decode(event->payload, event->payload_len, runtime->acquire_control_lease.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->acquire_control_lease.response_scratch->has_operation_id || runtime->acquire_control_lease.response_scratch->operation_id == 0U || !runtime->acquire_control_lease.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->acquire_control_lease.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->acquire_control_lease.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case CLEAR_ERROR_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->clear_error.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = clear_error_request_decode(event->payload, event->payload_len, runtime->clear_error.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->clear_error.request_scratch->has_operation_id || runtime->clear_error.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->clear_error.request_scratch->operation_id;
      result.detail.rpc.codec_status = clear_error_request_wlc_detail_fingerprint(runtime->clear_error.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = CLEAR_ERROR_REQUEST_MESSAGE_ID;
      identity.response_message_id = CLEAR_ERROR_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->clear_error.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->clear_error.request_handler(runtime->clear_error.user_data, runtime->clear_error.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case CLEAR_ERROR_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->clear_error.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = clear_error_response_decode(event->payload, event->payload_len, runtime->clear_error.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->clear_error.response_scratch->has_operation_id || runtime->clear_error.response_scratch->operation_id == 0U || !runtime->clear_error.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->clear_error.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->clear_error.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, CLEAR_ERROR_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case CLEAR_FAULTS_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->clear_faults.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = clear_faults_request_decode(event->payload, event->payload_len, runtime->clear_faults.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->clear_faults.request_scratch->has_operation_id || runtime->clear_faults.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->clear_faults.request_scratch->operation_id;
      result.detail.rpc.codec_status = clear_faults_request_wlc_detail_fingerprint(runtime->clear_faults.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = CLEAR_FAULTS_REQUEST_MESSAGE_ID;
      identity.response_message_id = CLEAR_FAULTS_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->clear_faults.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->clear_faults.request_handler(runtime->clear_faults.user_data, runtime->clear_faults.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case CLEAR_FAULTS_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->clear_faults.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = clear_faults_response_decode(event->payload, event->payload_len, runtime->clear_faults.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->clear_faults.response_scratch->has_operation_id || runtime->clear_faults.response_scratch->operation_id == 0U || !runtime->clear_faults.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->clear_faults.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->clear_faults.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, CLEAR_FAULTS_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case EMERGENCY_STOP_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->emergency_stop.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = emergency_stop_request_decode(event->payload, event->payload_len, runtime->emergency_stop.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->emergency_stop.request_scratch->has_operation_id || runtime->emergency_stop.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->emergency_stop.request_scratch->operation_id;
      result.detail.rpc.codec_status = emergency_stop_request_wlc_detail_fingerprint(runtime->emergency_stop.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = EMERGENCY_STOP_REQUEST_MESSAGE_ID;
      identity.response_message_id = EMERGENCY_STOP_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->emergency_stop.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->emergency_stop.request_handler(runtime->emergency_stop.user_data, runtime->emergency_stop.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case EMERGENCY_STOP_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->emergency_stop.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = emergency_stop_response_decode(event->payload, event->payload_len, runtime->emergency_stop.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->emergency_stop.response_scratch->has_operation_id || runtime->emergency_stop.response_scratch->operation_id == 0U || !runtime->emergency_stop.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->emergency_stop.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->emergency_stop.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, EMERGENCY_STOP_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case GET_DEVICE_INFO_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->get_device_info.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = get_device_info_request_decode(event->payload, event->payload_len, runtime->get_device_info.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->get_device_info.request_scratch->has_operation_id || runtime->get_device_info.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->get_device_info.request_scratch->operation_id;
      result.detail.rpc.codec_status = get_device_info_request_wlc_detail_fingerprint(runtime->get_device_info.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = GET_DEVICE_INFO_REQUEST_MESSAGE_ID;
      identity.response_message_id = GET_DEVICE_INFO_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->get_device_info.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->get_device_info.request_handler(runtime->get_device_info.user_data, runtime->get_device_info.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case GET_DEVICE_INFO_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->get_device_info.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = get_device_info_response_decode(event->payload, event->payload_len, runtime->get_device_info.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->get_device_info.response_scratch->has_operation_id || runtime->get_device_info.response_scratch->operation_id == 0U || !runtime->get_device_info.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->get_device_info.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->get_device_info.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, GET_DEVICE_INFO_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->get_device_settings.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = get_device_settings_request_decode(event->payload, event->payload_len, runtime->get_device_settings.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->get_device_settings.request_scratch->has_operation_id || runtime->get_device_settings.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->get_device_settings.request_scratch->operation_id;
      result.detail.rpc.codec_status = get_device_settings_request_wlc_detail_fingerprint(runtime->get_device_settings.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID;
      identity.response_message_id = GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->get_device_settings.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->get_device_settings.request_handler(runtime->get_device_settings.user_data, runtime->get_device_settings.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->get_device_settings.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = get_device_settings_response_decode(event->payload, event->payload_len, runtime->get_device_settings.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->get_device_settings.response_scratch->has_operation_id || runtime->get_device_settings.response_scratch->operation_id == 0U || !runtime->get_device_settings.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->get_device_settings.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->get_device_settings.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->get_motor_feedback.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = get_motor_feedback_request_decode(event->payload, event->payload_len, runtime->get_motor_feedback.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->get_motor_feedback.request_scratch->has_operation_id || runtime->get_motor_feedback.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->get_motor_feedback.request_scratch->operation_id;
      result.detail.rpc.codec_status = get_motor_feedback_request_wlc_detail_fingerprint(runtime->get_motor_feedback.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID;
      identity.response_message_id = GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->get_motor_feedback.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->get_motor_feedback.request_handler(runtime->get_motor_feedback.user_data, runtime->get_motor_feedback.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->get_motor_feedback.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = get_motor_feedback_response_decode(event->payload, event->payload_len, runtime->get_motor_feedback.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->get_motor_feedback.response_scratch->has_operation_id || runtime->get_motor_feedback.response_scratch->operation_id == 0U || !runtime->get_motor_feedback.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->get_motor_feedback.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->get_motor_feedback.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case HOME_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->home.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = home_request_decode(event->payload, event->payload_len, runtime->home.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->home.request_scratch->has_operation_id || runtime->home.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->home.request_scratch->operation_id;
      result.detail.rpc.codec_status = home_request_wlc_detail_fingerprint(runtime->home.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = HOME_REQUEST_MESSAGE_ID;
      identity.response_message_id = HOME_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->home.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->home.request_handler(runtime->home.user_data, runtime->home.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case HOME_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->home.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = home_response_decode(event->payload, event->payload_len, runtime->home.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->home.response_scratch->has_operation_id || runtime->home.response_scratch->operation_id == 0U || !runtime->home.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->home.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->home.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, HOME_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->motor_register_read.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_register_read_request_decode(event->payload, event->payload_len, runtime->motor_register_read.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_register_read.request_scratch->has_operation_id || runtime->motor_register_read.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_register_read.request_scratch->operation_id;
      result.detail.rpc.codec_status = motor_register_read_request_wlc_detail_fingerprint(runtime->motor_register_read.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID;
      identity.response_message_id = MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->motor_register_read.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->motor_register_read.request_handler(runtime->motor_register_read.user_data, runtime->motor_register_read.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->motor_register_read.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_register_read_response_decode(event->payload, event->payload_len, runtime->motor_register_read.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_register_read.response_scratch->has_operation_id || runtime->motor_register_read.response_scratch->operation_id == 0U || !runtime->motor_register_read.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_register_read.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->motor_register_read.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->motor_register_write.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_register_write_request_decode(event->payload, event->payload_len, runtime->motor_register_write.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_register_write.request_scratch->has_operation_id || runtime->motor_register_write.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_register_write.request_scratch->operation_id;
      result.detail.rpc.codec_status = motor_register_write_request_wlc_detail_fingerprint(runtime->motor_register_write.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID;
      identity.response_message_id = MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->motor_register_write.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->motor_register_write.request_handler(runtime->motor_register_write.user_data, runtime->motor_register_write.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->motor_register_write.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_register_write_response_decode(event->payload, event->payload_len, runtime->motor_register_write.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_register_write.response_scratch->has_operation_id || runtime->motor_register_write.response_scratch->operation_id == 0U || !runtime->motor_register_write.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_register_write.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->motor_register_write.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case MOTOR_SET_ZERO_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->motor_set_zero.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_set_zero_request_decode(event->payload, event->payload_len, runtime->motor_set_zero.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_set_zero.request_scratch->has_operation_id || runtime->motor_set_zero.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_set_zero.request_scratch->operation_id;
      result.detail.rpc.codec_status = motor_set_zero_request_wlc_detail_fingerprint(runtime->motor_set_zero.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = MOTOR_SET_ZERO_REQUEST_MESSAGE_ID;
      identity.response_message_id = MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->motor_set_zero.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->motor_set_zero.request_handler(runtime->motor_set_zero.user_data, runtime->motor_set_zero.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->motor_set_zero.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_set_zero_response_decode(event->payload, event->payload_len, runtime->motor_set_zero.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_set_zero.response_scratch->has_operation_id || runtime->motor_set_zero.response_scratch->operation_id == 0U || !runtime->motor_set_zero.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_set_zero.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->motor_set_zero.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->motor_store_parameters.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_store_parameters_request_decode(event->payload, event->payload_len, runtime->motor_store_parameters.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_store_parameters.request_scratch->has_operation_id || runtime->motor_store_parameters.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_store_parameters.request_scratch->operation_id;
      result.detail.rpc.codec_status = motor_store_parameters_request_wlc_detail_fingerprint(runtime->motor_store_parameters.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID;
      identity.response_message_id = MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->motor_store_parameters.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->motor_store_parameters.request_handler(runtime->motor_store_parameters.user_data, runtime->motor_store_parameters.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->motor_store_parameters.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = motor_store_parameters_response_decode(event->payload, event->payload_len, runtime->motor_store_parameters.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->motor_store_parameters.response_scratch->has_operation_id || runtime->motor_store_parameters.response_scratch->operation_id == 0U || !runtime->motor_store_parameters.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->motor_store_parameters.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->motor_store_parameters.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->release_control_lease.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = release_control_lease_request_decode(event->payload, event->payload_len, runtime->release_control_lease.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->release_control_lease.request_scratch->has_operation_id || runtime->release_control_lease.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->release_control_lease.request_scratch->operation_id;
      result.detail.rpc.codec_status = release_control_lease_request_wlc_detail_fingerprint(runtime->release_control_lease.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID;
      identity.response_message_id = RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->release_control_lease.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->release_control_lease.request_handler(runtime->release_control_lease.user_data, runtime->release_control_lease.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->release_control_lease.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = release_control_lease_response_decode(event->payload, event->payload_len, runtime->release_control_lease.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->release_control_lease.response_scratch->has_operation_id || runtime->release_control_lease.response_scratch->operation_id == 0U || !runtime->release_control_lease.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->release_control_lease.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->release_control_lease.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->set_arm_control_mode.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_arm_control_mode_request_decode(event->payload, event->payload_len, runtime->set_arm_control_mode.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_arm_control_mode.request_scratch->has_operation_id || runtime->set_arm_control_mode.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_arm_control_mode.request_scratch->operation_id;
      result.detail.rpc.codec_status = set_arm_control_mode_request_wlc_detail_fingerprint(runtime->set_arm_control_mode.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID;
      identity.response_message_id = SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->set_arm_control_mode.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->set_arm_control_mode.request_handler(runtime->set_arm_control_mode.user_data, runtime->set_arm_control_mode.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->set_arm_control_mode.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_arm_control_mode_response_decode(event->payload, event->payload_len, runtime->set_arm_control_mode.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_arm_control_mode.response_scratch->has_operation_id || runtime->set_arm_control_mode.response_scratch->operation_id == 0U || !runtime->set_arm_control_mode.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_arm_control_mode.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->set_arm_control_mode.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case SET_ARM_MODE_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->set_arm_mode.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_arm_mode_request_decode(event->payload, event->payload_len, runtime->set_arm_mode.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_arm_mode.request_scratch->has_operation_id || runtime->set_arm_mode.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_arm_mode.request_scratch->operation_id;
      result.detail.rpc.codec_status = set_arm_mode_request_wlc_detail_fingerprint(runtime->set_arm_mode.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = SET_ARM_MODE_REQUEST_MESSAGE_ID;
      identity.response_message_id = SET_ARM_MODE_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->set_arm_mode.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->set_arm_mode.request_handler(runtime->set_arm_mode.user_data, runtime->set_arm_mode.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case SET_ARM_MODE_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->set_arm_mode.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_arm_mode_response_decode(event->payload, event->payload_len, runtime->set_arm_mode.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_arm_mode.response_scratch->has_operation_id || runtime->set_arm_mode.response_scratch->operation_id == 0U || !runtime->set_arm_mode.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_arm_mode.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->set_arm_mode.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, SET_ARM_MODE_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case SET_DEVICE_INFO_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->set_device_info.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_device_info_request_decode(event->payload, event->payload_len, runtime->set_device_info.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_device_info.request_scratch->has_operation_id || runtime->set_device_info.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_device_info.request_scratch->operation_id;
      result.detail.rpc.codec_status = set_device_info_request_wlc_detail_fingerprint(runtime->set_device_info.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = SET_DEVICE_INFO_REQUEST_MESSAGE_ID;
      identity.response_message_id = SET_DEVICE_INFO_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->set_device_info.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->set_device_info.request_handler(runtime->set_device_info.user_data, runtime->set_device_info.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case SET_DEVICE_INFO_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->set_device_info.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_device_info_response_decode(event->payload, event->payload_len, runtime->set_device_info.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_device_info.response_scratch->has_operation_id || runtime->set_device_info.response_scratch->operation_id == 0U || !runtime->set_device_info.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_device_info.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->set_device_info.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, SET_DEVICE_INFO_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->set_device_settings.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_device_settings_request_decode(event->payload, event->payload_len, runtime->set_device_settings.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_device_settings.request_scratch->has_operation_id || runtime->set_device_settings.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_device_settings.request_scratch->operation_id;
      result.detail.rpc.codec_status = set_device_settings_request_wlc_detail_fingerprint(runtime->set_device_settings.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID;
      identity.response_message_id = SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->set_device_settings.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->set_device_settings.request_handler(runtime->set_device_settings.user_data, runtime->set_device_settings.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->set_device_settings.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_device_settings_response_decode(event->payload, event->payload_len, runtime->set_device_settings.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_device_settings.response_scratch->has_operation_id || runtime->set_device_settings.response_scratch->operation_id == 0U || !runtime->set_device_settings.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_device_settings.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->set_device_settings.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->set_gripper_control_mode.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_gripper_control_mode_request_decode(event->payload, event->payload_len, runtime->set_gripper_control_mode.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_gripper_control_mode.request_scratch->has_operation_id || runtime->set_gripper_control_mode.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_gripper_control_mode.request_scratch->operation_id;
      result.detail.rpc.codec_status = set_gripper_control_mode_request_wlc_detail_fingerprint(runtime->set_gripper_control_mode.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID;
      identity.response_message_id = SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->set_gripper_control_mode.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->set_gripper_control_mode.request_handler(runtime->set_gripper_control_mode.user_data, runtime->set_gripper_control_mode.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->set_gripper_control_mode.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_gripper_control_mode_response_decode(event->payload, event->payload_len, runtime->set_gripper_control_mode.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_gripper_control_mode.response_scratch->has_operation_id || runtime->set_gripper_control_mode.response_scratch->operation_id == 0U || !runtime->set_gripper_control_mode.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_gripper_control_mode.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->set_gripper_control_mode.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    case SET_ZERO_REQUEST_MESSAGE_ID: {
      wl_rpc_request_identity_t identity = {.request_fingerprint = fci_arm_rpc_fingerprint_seed};
      wl_rpc_server_request_t server_request = {0};
      wl_rpc_server_response_t replay = {0};
      size_t canonical_length = 0U;
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_server == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (event->peer_session_id != 0U && runtime->rpc_peer.session_id != event->peer_session_id) {
        wl_rpc_peer_observation_t observation = {0};
        result.detail.rpc.rpc_result = fci_arm_runtime_peer_observe(ctx, runtime, event->peer_session_id, &observation);
        if (result.detail.rpc.rpc_result != WL_RPC_OK) {
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        }
        if (observation.changed != 0U) result.detail.rpc.peer_changed = 1U;
      }
      if (runtime->set_zero.request_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_zero_request_decode(event->payload, event->payload_len, runtime->set_zero.request_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_zero.request_scratch->has_operation_id || runtime->set_zero.request_scratch->operation_id == 0U) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_zero.request_scratch->operation_id;
      result.detail.rpc.codec_status = set_zero_request_wlc_detail_fingerprint(runtime->set_zero.request_scratch, &identity.request_fingerprint, &canonical_length);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      result.detail.rpc.payload_length = canonical_length;
      identity.operation_id = result.detail.rpc.operation_id;
      identity.request_message_id = SET_ZERO_REQUEST_MESSAGE_ID;
      identity.response_message_id = SET_ZERO_RESPONSE_MESSAGE_ID;
      identity.peer_session_id = event->peer_session_id;
      result.detail.rpc.rpc_result = wl_rpc_server_begin(runtime->rpc_server, &identity, now_ms, &result.detail.rpc.rpc_disposition, &server_request, &replay);
      if (result.detail.rpc.rpc_result != WL_RPC_OK) {
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      switch (result.detail.rpc.rpc_disposition) {
        case WL_RPC_SERVER_NEW:
          result.detail.rpc.server_request = server_request;
          if (runtime->set_zero.request_handler == NULL) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
            break;
          }
          result.detail.rpc.application_result = runtime->set_zero.request_handler(runtime->set_zero.user_data, runtime->set_zero.request_scratch, &server_request, WL_DELIVERY_RELIABLE);
          if (result.detail.rpc.application_result != 0) {
            result.detail.rpc.rpc_result = wl_rpc_server_abandon(runtime->rpc_server, &server_request);
            result.domain = FCI_ARM_RUNTIME_APPLICATION_ERROR;
          } else {
            result.domain = FCI_ARM_RUNTIME_OK;
          }
          break;
        case WL_RPC_SERVER_PENDING_DUPLICATE:
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_REPLAY:
          result.detail.rpc.server_response = replay;
          result.detail.rpc.application_result = replay.application_status;
          result.detail.rpc.payload_length = replay.response_length;
          result.detail.rpc.core_result = WL_OK;
          result.domain = FCI_ARM_RUNTIME_OK;
          break;
        case WL_RPC_SERVER_CONFLICT:
          result.detail.rpc.rpc_result = WL_RPC_ERR_OPERATION_CONFLICT;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
        default:
          result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
          result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
          break;
      }
      break;
    }
    case SET_ZERO_RESPONSE_MESSAGE_ID: {
      result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
      if (event->type != WL_EVT_RELIABLE_RX) {
        result.domain = FCI_ARM_RUNTIME_DELIVERY_MISMATCH;
        break;
      }
      if (runtime->rpc_client == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_ROUTE;
        break;
      }
      if (runtime->set_zero.response_scratch == NULL) {
        result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
        break;
      }
      result.detail.rpc.codec_status = set_zero_response_decode(event->payload, event->payload_len, runtime->set_zero.response_scratch);
      if (result.detail.rpc.codec_status != WL_CODEC_OK) {
        result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
        break;
      }
      if (!runtime->set_zero.response_scratch->has_operation_id || runtime->set_zero.response_scratch->operation_id == 0U || !runtime->set_zero.response_scratch->has_status) {
        result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
        result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
        break;
      }
      result.detail.rpc.operation_id = runtime->set_zero.response_scratch->operation_id;
      result.detail.rpc.application_result = (int32_t)runtime->set_zero.response_scratch->status;
      result.detail.rpc.payload_length = event->payload_len;
      result.detail.rpc.rpc_result = wl_rpc_client_on_response(runtime->rpc_client, SET_ZERO_RESPONSE_MESSAGE_ID, result.detail.rpc.operation_id, result.detail.rpc.application_result, event->payload, event->payload_len);
      result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
      break;
    }
    default:
      result.domain = FCI_ARM_RUNTIME_UNKNOWN_MESSAGE;
      break;
  }

release_event:
  wl_event_release(ctx, event);
  result.event_consumed = 1U;
  return result;
}

int fci_arm_arm_status_latest_acquire(fci_arm_runtime_t *runtime, fci_arm_arm_status_latest_view_t *out_view) {
  wl_latest_view_t lease = {0};
  int result;
  if (out_view != NULL) memset(out_view, 0, sizeof(*out_view));
  if (runtime == NULL || out_view == NULL) return WL_ERR_INVALID_ARG;
  if (runtime->arm_status_latest == NULL) return WL_ERR_NOT_INITIALIZED;
  result = wl_latest_read_acquire(runtime->arm_status_latest, &lease);
  if (result != WL_OK) return result;
  if (lease.value == NULL || lease.value_size < sizeof(arm_status_t) || ((uintptr_t)lease.value % _Alignof(arm_status_t)) != 0U) {
    int failure = lease.value_size < sizeof(arm_status_t) ? WL_ERR_BUF_TOO_SMALL : WL_ERR_INVALID_STATE;
    int release_result = wl_latest_read_release(runtime->arm_status_latest, &lease);
    if (release_result != WL_OK) return release_result;
    return failure;
  }
  out_view->value = (const arm_status_t *)lease.value;
  out_view->generation = lease.generation;
  out_view->lease = lease;
  return WL_OK;
}

int fci_arm_arm_status_latest_release(fci_arm_runtime_t *runtime, fci_arm_arm_status_latest_view_t *view) {
  int result;
  if (runtime == NULL || view == NULL) return WL_ERR_INVALID_ARG;
  if (runtime->arm_status_latest == NULL) return WL_ERR_NOT_INITIALIZED;
  if ((const void *)view->value != view->lease.value || view->generation != view->lease.generation) return WL_ERR_INVALID_STATE;
  result = wl_latest_read_release(runtime->arm_status_latest, &view->lease);
  if (result == WL_OK) memset(view, 0, sizeof(*view));
  return result;
}

int fci_arm_motor_feedback_latest_acquire(fci_arm_runtime_t *runtime, fci_arm_motor_feedback_latest_view_t *out_view) {
  wl_latest_view_t lease = {0};
  int result;
  if (out_view != NULL) memset(out_view, 0, sizeof(*out_view));
  if (runtime == NULL || out_view == NULL) return WL_ERR_INVALID_ARG;
  if (runtime->motor_feedback_latest == NULL) return WL_ERR_NOT_INITIALIZED;
  result = wl_latest_read_acquire(runtime->motor_feedback_latest, &lease);
  if (result != WL_OK) return result;
  if (lease.value == NULL || lease.value_size < sizeof(motor_feedback_t) || ((uintptr_t)lease.value % _Alignof(motor_feedback_t)) != 0U) {
    int failure = lease.value_size < sizeof(motor_feedback_t) ? WL_ERR_BUF_TOO_SMALL : WL_ERR_INVALID_STATE;
    int release_result = wl_latest_read_release(runtime->motor_feedback_latest, &lease);
    if (release_result != WL_OK) return release_result;
    return failure;
  }
  out_view->value = (const motor_feedback_t *)lease.value;
  out_view->generation = lease.generation;
  out_view->lease = lease;
  return WL_OK;
}

int fci_arm_motor_feedback_latest_release(fci_arm_runtime_t *runtime, fci_arm_motor_feedback_latest_view_t *view) {
  int result;
  if (runtime == NULL || view == NULL) return WL_ERR_INVALID_ARG;
  if (runtime->motor_feedback_latest == NULL) return WL_ERR_NOT_INITIALIZED;
  if ((const void *)view->value != view->lease.value || view->generation != view->lease.generation) return WL_ERR_INVALID_STATE;
  result = wl_latest_read_release(runtime->motor_feedback_latest, &view->lease);
  if (result == WL_OK) memset(view, 0, sizeof(*view));
  return result;
}

int fci_arm_arm_diagnostics_latest_acquire(fci_arm_runtime_t *runtime, fci_arm_arm_diagnostics_latest_view_t *out_view) {
  wl_latest_view_t lease = {0};
  int result;
  if (out_view != NULL) memset(out_view, 0, sizeof(*out_view));
  if (runtime == NULL || out_view == NULL) return WL_ERR_INVALID_ARG;
  if (runtime->arm_diagnostics_latest == NULL) return WL_ERR_NOT_INITIALIZED;
  result = wl_latest_read_acquire(runtime->arm_diagnostics_latest, &lease);
  if (result != WL_OK) return result;
  if (lease.value == NULL || lease.value_size < sizeof(arm_diagnostics_t) || ((uintptr_t)lease.value % _Alignof(arm_diagnostics_t)) != 0U) {
    int failure = lease.value_size < sizeof(arm_diagnostics_t) ? WL_ERR_BUF_TOO_SMALL : WL_ERR_INVALID_STATE;
    int release_result = wl_latest_read_release(runtime->arm_diagnostics_latest, &lease);
    if (release_result != WL_OK) return release_result;
    return failure;
  }
  out_view->value = (const arm_diagnostics_t *)lease.value;
  out_view->generation = lease.generation;
  out_view->lease = lease;
  return WL_OK;
}

int fci_arm_arm_diagnostics_latest_release(fci_arm_runtime_t *runtime, fci_arm_arm_diagnostics_latest_view_t *view) {
  int result;
  if (runtime == NULL || view == NULL) return WL_ERR_INVALID_ARG;
  if (runtime->arm_diagnostics_latest == NULL) return WL_ERR_NOT_INITIALIZED;
  if ((const void *)view->value != view->lease.value || view->generation != view->lease.generation) return WL_ERR_INVALID_STATE;
  result = wl_latest_read_release(runtime->arm_diagnostics_latest, &view->lease);
  if (result == WL_OK) memset(view, 0, sizeof(*view));
  return result;
}

static fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const acquire_control_lease_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  acquire_control_lease_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->acquire_control_lease_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID, ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID, ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_acquire_control_lease_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_acquire_control_lease_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_acquire_control_lease_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID || out_client->response_message_id != ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_acquire_control_lease_client_decode(const wl_rpc_client_result_t *client, acquire_control_lease_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  acquire_control_lease_response_clear(response);
  if (client->request_message_id != ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID || client->response_message_id != ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = acquire_control_lease_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_acquire_control_lease_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID || client.response_message_id != ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const acquire_control_lease_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  acquire_control_lease_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->acquire_control_lease_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = acquire_control_lease_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const acquire_control_lease_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_acquire_control_lease_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_acquire_control_lease_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const acquire_control_lease_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_acquire_control_lease_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_clear_error_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = CLEAR_ERROR_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_clear_error_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const clear_error_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  clear_error_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = CLEAR_ERROR_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->clear_error_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, CLEAR_ERROR_REQUEST_MESSAGE_ID, CLEAR_ERROR_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, CLEAR_ERROR_REQUEST_MESSAGE_ID, CLEAR_ERROR_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_clear_error_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_clear_error_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_clear_error_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != CLEAR_ERROR_REQUEST_MESSAGE_ID || out_client->response_message_id != CLEAR_ERROR_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_clear_error_client_decode(const wl_rpc_client_result_t *client, clear_error_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = CLEAR_ERROR_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  clear_error_response_clear(response);
  if (client->request_message_id != CLEAR_ERROR_REQUEST_MESSAGE_ID || client->response_message_id != CLEAR_ERROR_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = clear_error_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_clear_error_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != CLEAR_ERROR_REQUEST_MESSAGE_ID || client.response_message_id != CLEAR_ERROR_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_clear_error_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const clear_error_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  clear_error_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = CLEAR_ERROR_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != CLEAR_ERROR_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != CLEAR_ERROR_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->clear_error_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = clear_error_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_clear_error_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const clear_error_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_clear_error_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_clear_error_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const clear_error_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_clear_error_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_clear_faults_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = CLEAR_FAULTS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_clear_faults_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const clear_faults_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  clear_faults_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = CLEAR_FAULTS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->clear_faults_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, CLEAR_FAULTS_REQUEST_MESSAGE_ID, CLEAR_FAULTS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, CLEAR_FAULTS_REQUEST_MESSAGE_ID, CLEAR_FAULTS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_clear_faults_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_clear_faults_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_clear_faults_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != CLEAR_FAULTS_REQUEST_MESSAGE_ID || out_client->response_message_id != CLEAR_FAULTS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_clear_faults_client_decode(const wl_rpc_client_result_t *client, clear_faults_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = CLEAR_FAULTS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  clear_faults_response_clear(response);
  if (client->request_message_id != CLEAR_FAULTS_REQUEST_MESSAGE_ID || client->response_message_id != CLEAR_FAULTS_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = clear_faults_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_clear_faults_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != CLEAR_FAULTS_REQUEST_MESSAGE_ID || client.response_message_id != CLEAR_FAULTS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_clear_faults_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const clear_faults_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  clear_faults_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = CLEAR_FAULTS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != CLEAR_FAULTS_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != CLEAR_FAULTS_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->clear_faults_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = clear_faults_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_clear_faults_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const clear_faults_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_clear_faults_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_clear_faults_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const clear_faults_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_clear_faults_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_emergency_stop_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = EMERGENCY_STOP_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_emergency_stop_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const emergency_stop_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  emergency_stop_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = EMERGENCY_STOP_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->emergency_stop_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, EMERGENCY_STOP_REQUEST_MESSAGE_ID, EMERGENCY_STOP_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, EMERGENCY_STOP_REQUEST_MESSAGE_ID, EMERGENCY_STOP_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_emergency_stop_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_emergency_stop_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_emergency_stop_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != EMERGENCY_STOP_REQUEST_MESSAGE_ID || out_client->response_message_id != EMERGENCY_STOP_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_emergency_stop_client_decode(const wl_rpc_client_result_t *client, emergency_stop_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = EMERGENCY_STOP_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  emergency_stop_response_clear(response);
  if (client->request_message_id != EMERGENCY_STOP_REQUEST_MESSAGE_ID || client->response_message_id != EMERGENCY_STOP_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = emergency_stop_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_emergency_stop_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != EMERGENCY_STOP_REQUEST_MESSAGE_ID || client.response_message_id != EMERGENCY_STOP_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_emergency_stop_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const emergency_stop_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  emergency_stop_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = EMERGENCY_STOP_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != EMERGENCY_STOP_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != EMERGENCY_STOP_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->emergency_stop_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = emergency_stop_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_emergency_stop_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const emergency_stop_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_emergency_stop_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_emergency_stop_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const emergency_stop_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_emergency_stop_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_get_device_info_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = GET_DEVICE_INFO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_get_device_info_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const get_device_info_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  get_device_info_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = GET_DEVICE_INFO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->get_device_info_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, GET_DEVICE_INFO_REQUEST_MESSAGE_ID, GET_DEVICE_INFO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, GET_DEVICE_INFO_REQUEST_MESSAGE_ID, GET_DEVICE_INFO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_get_device_info_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_get_device_info_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_get_device_info_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != GET_DEVICE_INFO_REQUEST_MESSAGE_ID || out_client->response_message_id != GET_DEVICE_INFO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_get_device_info_client_decode(const wl_rpc_client_result_t *client, get_device_info_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = GET_DEVICE_INFO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  get_device_info_response_clear(response);
  if (client->request_message_id != GET_DEVICE_INFO_REQUEST_MESSAGE_ID || client->response_message_id != GET_DEVICE_INFO_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = get_device_info_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_get_device_info_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != GET_DEVICE_INFO_REQUEST_MESSAGE_ID || client.response_message_id != GET_DEVICE_INFO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_get_device_info_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_device_info_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  get_device_info_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = GET_DEVICE_INFO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != GET_DEVICE_INFO_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != GET_DEVICE_INFO_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->get_device_info_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = get_device_info_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_get_device_info_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const get_device_info_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_get_device_info_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_get_device_info_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_device_info_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_get_device_info_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_get_device_settings_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_get_device_settings_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const get_device_settings_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  get_device_settings_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->get_device_settings_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID, GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID, GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_get_device_settings_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_get_device_settings_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_get_device_settings_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || out_client->response_message_id != GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_get_device_settings_client_decode(const wl_rpc_client_result_t *client, get_device_settings_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  get_device_settings_response_clear(response);
  if (client->request_message_id != GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || client->response_message_id != GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = get_device_settings_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_get_device_settings_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || client.response_message_id != GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_get_device_settings_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_device_settings_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  get_device_settings_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->get_device_settings_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = get_device_settings_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_get_device_settings_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const get_device_settings_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_get_device_settings_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_get_device_settings_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_device_settings_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_get_device_settings_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const get_motor_feedback_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  get_motor_feedback_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->get_motor_feedback_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID, GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID, GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_get_motor_feedback_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_get_motor_feedback_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_get_motor_feedback_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID || out_client->response_message_id != GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_get_motor_feedback_client_decode(const wl_rpc_client_result_t *client, get_motor_feedback_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  get_motor_feedback_response_clear(response);
  if (client->request_message_id != GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID || client->response_message_id != GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = get_motor_feedback_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_get_motor_feedback_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID || client.response_message_id != GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_motor_feedback_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  get_motor_feedback_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->get_motor_feedback_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = get_motor_feedback_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const get_motor_feedback_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_get_motor_feedback_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_get_motor_feedback_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const get_motor_feedback_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_get_motor_feedback_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_home_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = HOME_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_home_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const home_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  home_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = HOME_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->home_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, HOME_REQUEST_MESSAGE_ID, HOME_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, HOME_REQUEST_MESSAGE_ID, HOME_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_home_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_home_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_home_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != HOME_REQUEST_MESSAGE_ID || out_client->response_message_id != HOME_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_home_client_decode(const wl_rpc_client_result_t *client, home_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = HOME_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  home_response_clear(response);
  if (client->request_message_id != HOME_REQUEST_MESSAGE_ID || client->response_message_id != HOME_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = home_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_home_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != HOME_REQUEST_MESSAGE_ID || client.response_message_id != HOME_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_home_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const home_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  home_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = HOME_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != HOME_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != HOME_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->home_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = home_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_home_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const home_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_home_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_home_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const home_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_home_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_motor_register_read_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_register_read_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_register_read_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  motor_register_read_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->motor_register_read_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID, MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID, MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_motor_register_read_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_motor_register_read_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_motor_register_read_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID || out_client->response_message_id != MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_motor_register_read_client_decode(const wl_rpc_client_result_t *client, motor_register_read_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  motor_register_read_response_clear(response);
  if (client->request_message_id != MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID || client->response_message_id != MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = motor_register_read_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_motor_register_read_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID || client.response_message_id != MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_motor_register_read_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_register_read_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  motor_register_read_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->motor_register_read_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = motor_register_read_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_register_read_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_register_read_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_register_read_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_motor_register_read_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_register_read_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_register_read_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_motor_register_write_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_register_write_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_register_write_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  motor_register_write_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->motor_register_write_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID, MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID, MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_motor_register_write_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_motor_register_write_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_motor_register_write_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID || out_client->response_message_id != MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_motor_register_write_client_decode(const wl_rpc_client_result_t *client, motor_register_write_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  motor_register_write_response_clear(response);
  if (client->request_message_id != MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID || client->response_message_id != MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = motor_register_write_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_motor_register_write_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID || client.response_message_id != MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_motor_register_write_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_register_write_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  motor_register_write_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->motor_register_write_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = motor_register_write_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_register_write_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_register_write_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_register_write_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_motor_register_write_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_register_write_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_register_write_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_motor_set_zero_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_SET_ZERO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_set_zero_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_set_zero_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  motor_set_zero_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = MOTOR_SET_ZERO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->motor_set_zero_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, MOTOR_SET_ZERO_REQUEST_MESSAGE_ID, MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, MOTOR_SET_ZERO_REQUEST_MESSAGE_ID, MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_motor_set_zero_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_motor_set_zero_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_motor_set_zero_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != MOTOR_SET_ZERO_REQUEST_MESSAGE_ID || out_client->response_message_id != MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_motor_set_zero_client_decode(const wl_rpc_client_result_t *client, motor_set_zero_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  motor_set_zero_response_clear(response);
  if (client->request_message_id != MOTOR_SET_ZERO_REQUEST_MESSAGE_ID || client->response_message_id != MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = motor_set_zero_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_motor_set_zero_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != MOTOR_SET_ZERO_REQUEST_MESSAGE_ID || client.response_message_id != MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_motor_set_zero_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_set_zero_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  motor_set_zero_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != MOTOR_SET_ZERO_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->motor_set_zero_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = motor_set_zero_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_set_zero_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_set_zero_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_set_zero_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_motor_set_zero_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_set_zero_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_set_zero_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const motor_store_parameters_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  motor_store_parameters_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->motor_store_parameters_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID, MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID, MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_motor_store_parameters_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_motor_store_parameters_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_motor_store_parameters_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID || out_client->response_message_id != MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_motor_store_parameters_client_decode(const wl_rpc_client_result_t *client, motor_store_parameters_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  motor_store_parameters_response_clear(response);
  if (client->request_message_id != MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID || client->response_message_id != MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = motor_store_parameters_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_motor_store_parameters_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID || client.response_message_id != MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_store_parameters_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  motor_store_parameters_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->motor_store_parameters_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = motor_store_parameters_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const motor_store_parameters_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_store_parameters_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_motor_store_parameters_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const motor_store_parameters_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_motor_store_parameters_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_release_control_lease_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_release_control_lease_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const release_control_lease_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  release_control_lease_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->release_control_lease_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID, RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID, RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_release_control_lease_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_release_control_lease_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_release_control_lease_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID || out_client->response_message_id != RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_release_control_lease_client_decode(const wl_rpc_client_result_t *client, release_control_lease_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  release_control_lease_response_clear(response);
  if (client->request_message_id != RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID || client->response_message_id != RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = release_control_lease_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_release_control_lease_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID || client.response_message_id != RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_release_control_lease_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const release_control_lease_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  release_control_lease_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->release_control_lease_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = release_control_lease_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_release_control_lease_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const release_control_lease_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_release_control_lease_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_release_control_lease_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const release_control_lease_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_release_control_lease_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_arm_control_mode_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  set_arm_control_mode_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->set_arm_control_mode_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID, SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID, SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_set_arm_control_mode_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_set_arm_control_mode_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_set_arm_control_mode_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID || out_client->response_message_id != SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_set_arm_control_mode_client_decode(const wl_rpc_client_result_t *client, set_arm_control_mode_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  set_arm_control_mode_response_clear(response);
  if (client->request_message_id != SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID || client->response_message_id != SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = set_arm_control_mode_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_set_arm_control_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID || client.response_message_id != SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_arm_control_mode_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  set_arm_control_mode_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->set_arm_control_mode_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = set_arm_control_mode_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_arm_control_mode_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_arm_control_mode_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_set_arm_control_mode_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_arm_control_mode_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_arm_control_mode_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_set_arm_mode_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_ARM_MODE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_arm_mode_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_arm_mode_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  set_arm_mode_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = SET_ARM_MODE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->set_arm_mode_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, SET_ARM_MODE_REQUEST_MESSAGE_ID, SET_ARM_MODE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, SET_ARM_MODE_REQUEST_MESSAGE_ID, SET_ARM_MODE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_set_arm_mode_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_set_arm_mode_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_set_arm_mode_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != SET_ARM_MODE_REQUEST_MESSAGE_ID || out_client->response_message_id != SET_ARM_MODE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_set_arm_mode_client_decode(const wl_rpc_client_result_t *client, set_arm_mode_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_ARM_MODE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  set_arm_mode_response_clear(response);
  if (client->request_message_id != SET_ARM_MODE_REQUEST_MESSAGE_ID || client->response_message_id != SET_ARM_MODE_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = set_arm_mode_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_set_arm_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != SET_ARM_MODE_REQUEST_MESSAGE_ID || client.response_message_id != SET_ARM_MODE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_set_arm_mode_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_arm_mode_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  set_arm_mode_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = SET_ARM_MODE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != SET_ARM_MODE_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != SET_ARM_MODE_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->set_arm_mode_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = set_arm_mode_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_arm_mode_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_arm_mode_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_arm_mode_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_set_arm_mode_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_arm_mode_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_arm_mode_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_set_device_info_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_DEVICE_INFO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_device_info_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_device_info_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  set_device_info_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = SET_DEVICE_INFO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->set_device_info_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, SET_DEVICE_INFO_REQUEST_MESSAGE_ID, SET_DEVICE_INFO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, SET_DEVICE_INFO_REQUEST_MESSAGE_ID, SET_DEVICE_INFO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_set_device_info_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_set_device_info_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_set_device_info_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != SET_DEVICE_INFO_REQUEST_MESSAGE_ID || out_client->response_message_id != SET_DEVICE_INFO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_set_device_info_client_decode(const wl_rpc_client_result_t *client, set_device_info_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_DEVICE_INFO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  set_device_info_response_clear(response);
  if (client->request_message_id != SET_DEVICE_INFO_REQUEST_MESSAGE_ID || client->response_message_id != SET_DEVICE_INFO_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = set_device_info_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_set_device_info_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != SET_DEVICE_INFO_REQUEST_MESSAGE_ID || client.response_message_id != SET_DEVICE_INFO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_set_device_info_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_device_info_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  set_device_info_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = SET_DEVICE_INFO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != SET_DEVICE_INFO_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != SET_DEVICE_INFO_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->set_device_info_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = set_device_info_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_device_info_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_device_info_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_device_info_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_set_device_info_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_device_info_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_device_info_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_set_device_settings_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_device_settings_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_device_settings_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  set_device_settings_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->set_device_settings_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID, SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID, SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_set_device_settings_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_set_device_settings_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_set_device_settings_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || out_client->response_message_id != SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_set_device_settings_client_decode(const wl_rpc_client_result_t *client, set_device_settings_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  set_device_settings_response_clear(response);
  if (client->request_message_id != SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || client->response_message_id != SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = set_device_settings_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_set_device_settings_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || client.response_message_id != SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_set_device_settings_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_device_settings_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  set_device_settings_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->set_device_settings_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = set_device_settings_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_device_settings_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_device_settings_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_device_settings_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_set_device_settings_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_device_settings_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_device_settings_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_gripper_control_mode_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  set_gripper_control_mode_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->set_gripper_control_mode_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID, SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID, SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_set_gripper_control_mode_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_set_gripper_control_mode_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_set_gripper_control_mode_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID || out_client->response_message_id != SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_client_decode(const wl_rpc_client_result_t *client, set_gripper_control_mode_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  set_gripper_control_mode_response_clear(response);
  if (client->request_message_id != SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID || client->response_message_id != SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = set_gripper_control_mode_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_set_gripper_control_mode_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID || client.response_message_id != SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_gripper_control_mode_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  set_gripper_control_mode_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->set_gripper_control_mode_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = set_gripper_control_mode_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_gripper_control_mode_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_gripper_control_mode_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_set_gripper_control_mode_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_gripper_control_mode_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_gripper_control_mode_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static fci_arm_runtime_result_t fci_arm_set_zero_client_finish_start(fci_arm_runtime_t *runtime, uint32_t operation_id, fci_arm_send_result_t sent) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_ZERO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.operation_id = operation_id;
  result.detail.rpc.codec_status = sent.codec_status;
  result.detail.rpc.core_result = sent.core_result;
  result.detail.rpc.handle = sent.handle;
  result.detail.rpc.payload_length = sent.payload_length;
  if (sent.domain == FCI_ARM_SEND_CODEC_ERROR || sent.domain == FCI_ARM_SEND_CORE_ERROR) {
    const int32_t link_result = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? WL_ERR_CORRUPT_PAYLOAD : sent.core_result;
    result.detail.rpc.rpc_result = wl_rpc_client_link_failed(runtime->rpc_client, operation_id, link_result);
    if (result.detail.rpc.rpc_result == WL_RPC_OK)
      result.detail.rpc.rpc_result = wl_rpc_client_release(runtime->rpc_client, operation_id);
    if (result.detail.rpc.rpc_result == WL_RPC_OK) result.detail.rpc.operation_id = 0U;
    result.domain = sent.domain == FCI_ARM_SEND_CODEC_ERROR ? FCI_ARM_RUNTIME_CODEC_ERROR : FCI_ARM_RUNTIME_CORE_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_client_bind_tx(runtime->rpc_client, operation_id, result.detail.rpc.handle);
  result.domain = result.detail.rpc.rpc_result == WL_RPC_OK ? FCI_ARM_RUNTIME_OK : FCI_ARM_RUNTIME_RPC_ERROR;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_zero_client_start(wl_ctx_t *ctx, fci_arm_runtime_t *runtime, const set_zero_request_t *request, uint32_t timeout_ms, wl_time_ms_t now_ms) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  fci_arm_send_result_t sent;
  set_zero_request_t *encoded_request;
  uint32_t operation_id = 0U;
  result.message_id = SET_ZERO_REQUEST_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (ctx == NULL || runtime == NULL || runtime->rpc_client == NULL || request == NULL) return result;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_request = &runtime->rpc_encode_scratch->set_zero_request;
  if ((const void *)request == (const void *)encoded_request) return result;
  if (request->has_operation_id && request->operation_id != 0U) {
    operation_id = request->operation_id;
    result.detail.rpc.rpc_result = wl_rpc_client_begin_with_id(runtime->rpc_client, operation_id, SET_ZERO_REQUEST_MESSAGE_ID, SET_ZERO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms);
  } else {
    result.detail.rpc.rpc_result = wl_rpc_client_begin(runtime->rpc_client, SET_ZERO_REQUEST_MESSAGE_ID, SET_ZERO_RESPONSE_MESSAGE_ID, timeout_ms, now_ms, &operation_id);
  }
  result.detail.rpc.operation_id = operation_id;
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_request = *request;
  encoded_request->has_operation_id = true;
  encoded_request->operation_id = operation_id;
  sent = fci_arm_set_zero_request_send(ctx, encoded_request, WL_DELIVERY_RELIABLE, now_ms);
  return fci_arm_set_zero_client_finish_start(runtime, operation_id, sent);
}

wl_rpc_err_t fci_arm_set_zero_client_inspect(const fci_arm_runtime_t *runtime, uint32_t operation_id, wl_rpc_client_result_t *out_client) {
  wl_rpc_err_t result;
  if (out_client != NULL) memset(out_client, 0, sizeof(*out_client));
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U || out_client == NULL) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, out_client);
  if (result != WL_RPC_OK) return result;
  if (out_client->request_message_id != SET_ZERO_REQUEST_MESSAGE_ID || out_client->response_message_id != SET_ZERO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return WL_RPC_OK;
}

fci_arm_runtime_result_t fci_arm_set_zero_client_decode(const wl_rpc_client_result_t *client, set_zero_response_t *response) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  result.message_id = SET_ZERO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  if (client != NULL) {
    result.detail.rpc.operation_id = client->operation_id;
    result.detail.rpc.handle = client->tx_handle;
    result.detail.rpc.core_result = client->link_result;
    result.detail.rpc.application_result = client->application_status;
    result.detail.rpc.payload_length = client->response_length;
  }
  if (client == NULL || response == NULL || client->operation_id == 0U) return result;
  set_zero_response_clear(response);
  if (client->request_message_id != SET_ZERO_REQUEST_MESSAGE_ID || client->response_message_id != SET_ZERO_RESPONSE_MESSAGE_ID) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  if ((client->state != WL_RPC_CLIENT_COMPLETED && client->state != WL_RPC_CLIENT_APPLICATION_ERROR) || client->response_data == NULL || client->response_length == 0U) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_STATE;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.codec_status = set_zero_response_decode(client->response_data, client->response_length, response);
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  if (!response->has_operation_id || response->operation_id != client->operation_id || !response->has_status || (int32_t)response->status != client->application_status) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_RESPONSE_MISMATCH;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

wl_rpc_err_t fci_arm_set_zero_client_release(fci_arm_runtime_t *runtime, uint32_t operation_id) {
  wl_rpc_client_result_t client = {0};
  wl_rpc_err_t result;
  if (runtime == NULL || runtime->rpc_client == NULL || operation_id == 0U) return WL_RPC_ERR_INVALID_ARG;
  result = wl_rpc_client_get(runtime->rpc_client, operation_id, &client);
  if (result != WL_RPC_OK) return result;
  if (client.request_message_id != SET_ZERO_REQUEST_MESSAGE_ID || client.response_message_id != SET_ZERO_RESPONSE_MESSAGE_ID) return WL_RPC_ERR_RESPONSE_MISMATCH;
  return wl_rpc_client_release(runtime->rpc_client, operation_id);
}

static fci_arm_runtime_result_t fci_arm_set_zero_server_finish(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_zero_response_t *response, wl_time_ms_t now_ms, bool reject) {
  fci_arm_runtime_result_t result = fci_arm_runtime_result(NULL);
  wl_rpc_server_response_buffer_t buffer = {0};
  wl_rpc_server_response_t cached = {0};
  set_zero_response_t *encoded_response;
  size_t encoded_length = 0U;
  result.message_id = SET_ZERO_RESPONSE_MESSAGE_ID;
  result.detail_kind = FCI_ARM_RUNTIME_DETAIL_RPC;
  result.detail.rpc.application_result = application_status;
  if (runtime == NULL || runtime->rpc_server == NULL || server_request == NULL || server_request->generation == 0U || server_request->identity.operation_id == 0U || server_request->identity.request_message_id != SET_ZERO_REQUEST_MESSAGE_ID || server_request->identity.response_message_id != SET_ZERO_RESPONSE_MESSAGE_ID || response == NULL) return result;
  result.detail.rpc.operation_id = server_request->identity.operation_id;
  result.detail.rpc.server_request = *server_request;
  if (runtime->rpc_encode_scratch == NULL) {
    result.domain = FCI_ARM_RUNTIME_MISSING_SCRATCH;
    return result;
  }
  encoded_response = &runtime->rpc_encode_scratch->set_zero_response;
  if ((const void *)response == (const void *)encoded_response) return result;
  if (reject && application_status == 0) {
    result.detail.rpc.rpc_result = WL_RPC_ERR_INVALID_ARG;
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_prepare(runtime->rpc_server, server_request, &buffer);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  *encoded_response = *response;
  encoded_response->has_operation_id = true;
  encoded_response->operation_id = server_request->identity.operation_id;
  encoded_response->has_status = true;
  encoded_response->status = application_status;
  result.detail.rpc.codec_status = set_zero_response_encode(encoded_response, buffer.data, buffer.capacity, &encoded_length);
  result.detail.rpc.payload_length = encoded_length;
  if (result.detail.rpc.codec_status != WL_CODEC_OK) {
    result.domain = FCI_ARM_RUNTIME_CODEC_ERROR;
    return result;
  }
  result.detail.rpc.rpc_result = wl_rpc_server_response_commit(runtime->rpc_server, &buffer, application_status, encoded_length, now_ms, &cached);
  if (result.detail.rpc.rpc_result != WL_RPC_OK) {
    result.domain = FCI_ARM_RUNTIME_RPC_ERROR;
    return result;
  }
  result.detail.rpc.server_response = cached;
  result.detail.rpc.application_result = cached.application_status;
  result.detail.rpc.payload_length = cached.response_length;
  result.detail.rpc.core_result = WL_OK;
  result.domain = FCI_ARM_RUNTIME_OK;
  return result;
}

fci_arm_runtime_result_t fci_arm_set_zero_server_complete(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, const set_zero_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_zero_server_finish(runtime, server_request, 0, response, now_ms, false);
}

fci_arm_runtime_result_t fci_arm_set_zero_server_reject(fci_arm_runtime_t *runtime, const wl_rpc_server_request_t *server_request, int32_t application_status, const set_zero_response_t *response, wl_time_ms_t now_ms) {
  return fci_arm_set_zero_server_finish(runtime, server_request, application_status, response, now_ms, true);
}

static wl_pump_event_disposition_t fci_arm_runtime_pump_event(void *user_data, wl_ctx_t *ctx, const wl_event_t *event, wl_time_ms_t now_ms) {
  fci_arm_runtime_pump_t *pump = (fci_arm_runtime_pump_t *)user_data;
  fci_arm_runtime_result_t result;
  if (pump == NULL || pump->runtime == NULL) return WL_PUMP_EVENT_UNHANDLED;
  result = fci_arm_runtime_dispatch_event(ctx, event, pump->runtime, now_ms);
  if (pump->on_result != NULL) pump->on_result(pump->user_data, &result);
  return result.event_consumed != 0U ? WL_PUMP_EVENT_CONSUMED : WL_PUMP_EVENT_UNHANDLED;
}

static uint8_t fci_arm_runtime_pump_progress(void *user_data, wl_ctx_t *ctx, wl_time_ms_t now_ms) {
  fci_arm_runtime_pump_t *pump = (fci_arm_runtime_pump_t *)user_data;
  if (pump == NULL || pump->runtime == NULL) return 0U;
  pump->last_service_result = fci_arm_runtime_service(ctx, pump->runtime, now_ms, &pump->last_service);
  if (pump->last_service_result != WL_RPC_OK) return 0U;
  if (pump->last_service.response.message_id != 0U && pump->on_result != NULL)
    pump->on_result(pump->user_data, &pump->last_service.response);
  return pump->last_service.responses_submitted != 0U ? 1U : 0U;
}

static uint32_t fci_arm_runtime_pump_deadline(const void *user_data, wl_time_ms_t now_ms) {
  const fci_arm_runtime_pump_t *pump = (const fci_arm_runtime_pump_t *)user_data;
  wl_rpc_deadline_hint_t hint = {0};
  if (pump == NULL || pump->runtime == NULL || fci_arm_runtime_get_deadline_hint(pump->runtime, now_ms, &hint) != WL_RPC_OK)
    return WL_POLL_NO_DEADLINE_MS;
  return hint.next_deadline_ms;
}

wl_err_t fci_arm_runtime_pump_init(fci_arm_runtime_pump_t *pump, fci_arm_runtime_t *runtime, fci_arm_runtime_result_fn on_result, void *user_data) {
  if (pump == NULL || runtime == NULL) return WL_ERR_INVALID_ARG;
  memset(pump, 0, sizeof(*pump));
  pump->runtime = runtime;
  pump->user_data = user_data;
  pump->on_result = on_result;
  return WL_OK;
}

wl_pump_hooks_t fci_arm_runtime_pump_hooks(fci_arm_runtime_pump_t *pump) {
  wl_pump_hooks_t hooks = {0};
  if (pump == NULL) return hooks;
  hooks.application_user_data = pump;
  hooks.on_event = fci_arm_runtime_pump_event;
  hooks.application_progress = fci_arm_runtime_pump_progress;
  hooks.application_deadline_hint = fci_arm_runtime_pump_deadline;
  return hooks;
}
