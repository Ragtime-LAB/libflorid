/* SPDX-License-Identifier: Apache-2.0 */
/* Explicit manual RPC opt-in. Never release a callback-managed call. */
#ifndef FCI_ARM_ADVANCED_H
#define FCI_ARM_ADVANCED_H
#include "fci_arm_runtime.h"
#if FCI_ARM_HAS_DEFAULT_ENDPOINT
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_acquire_control_lease_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  acquire_control_lease_response_t response;
} fci_arm_acquire_control_lease_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_call(fci_arm_endpoint_t *endpoint,
    const acquire_control_lease_request_t *request, uint32_t timeout_ms,
    fci_arm_acquire_control_lease_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_acquire_control_lease_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_acquire_control_lease_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_acquire_control_lease_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25097U && result->response_message_id == 25098U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_acquire_control_lease_call_t *call, fci_arm_acquire_control_lease_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_acquire_control_lease_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_acquire_control_lease_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_acquire_control_lease_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_acquire_control_lease_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_acquire_control_lease_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_acquire_control_lease_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_acquire_control_lease_request_token_t *token, const acquire_control_lease_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_acquire_control_lease_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_acquire_control_lease_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_acquire_control_lease_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_acquire_control_lease_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_acquire_control_lease_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_acquire_control_lease_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_error_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_clear_error_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  clear_error_response_t response;
} fci_arm_clear_error_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_error_call(fci_arm_endpoint_t *endpoint,
    const clear_error_request_t *request, uint32_t timeout_ms,
    fci_arm_clear_error_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_clear_error_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_clear_error_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_error_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_error_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 24841U && result->response_message_id == 24842U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_error_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_error_call_t *call, fci_arm_clear_error_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_clear_error_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_clear_error_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_error_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_error_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_clear_error_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_error_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_error_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_clear_error_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_error_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_error_request_token_t *token, const clear_error_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_clear_error_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_clear_error_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_clear_error_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_error_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_clear_error_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_clear_error_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_clear_faults_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  clear_faults_response_t response;
} fci_arm_clear_faults_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_call(fci_arm_endpoint_t *endpoint,
    const clear_faults_request_t *request, uint32_t timeout_ms,
    fci_arm_clear_faults_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_clear_faults_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_clear_faults_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_faults_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25093U && result->response_message_id == 25094U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_faults_call_t *call, fci_arm_clear_faults_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_clear_faults_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_clear_faults_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_faults_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_clear_faults_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_faults_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_clear_faults_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_faults_request_token_t *token, const clear_faults_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_clear_faults_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_clear_faults_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_clear_faults_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_clear_faults_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_clear_faults_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_clear_faults_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_emergency_stop_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  emergency_stop_response_t response;
} fci_arm_emergency_stop_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_call(fci_arm_endpoint_t *endpoint,
    const emergency_stop_request_t *request, uint32_t timeout_ms,
    fci_arm_emergency_stop_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_emergency_stop_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_emergency_stop_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_emergency_stop_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25347U && result->response_message_id == 25348U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_emergency_stop_call_t *call, fci_arm_emergency_stop_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_emergency_stop_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_emergency_stop_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_emergency_stop_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_emergency_stop_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_emergency_stop_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_emergency_stop_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_emergency_stop_request_token_t *token, const emergency_stop_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_emergency_stop_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_emergency_stop_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_emergency_stop_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_emergency_stop_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_emergency_stop_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_emergency_stop_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_get_device_info_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  get_device_info_response_t response;
} fci_arm_get_device_info_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_call(fci_arm_endpoint_t *endpoint,
    const get_device_info_request_t *request, uint32_t timeout_ms,
    fci_arm_get_device_info_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_device_info_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_get_device_info_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_info_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25109U && result->response_message_id == 25110U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_info_call_t *call, fci_arm_get_device_info_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_get_device_info_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_get_device_info_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_info_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_get_device_info_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_info_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_get_device_info_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_info_request_token_t *token, const get_device_info_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_device_info_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_get_device_info_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_get_device_info_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_info_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_device_info_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_get_device_info_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_get_device_settings_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  get_device_settings_response_t response;
} fci_arm_get_device_settings_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_call(fci_arm_endpoint_t *endpoint,
    const get_device_settings_request_t *request, uint32_t timeout_ms,
    fci_arm_get_device_settings_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_device_settings_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_get_device_settings_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_settings_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25128U && result->response_message_id == 25129U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_settings_call_t *call, fci_arm_get_device_settings_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_get_device_settings_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_get_device_settings_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_settings_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_get_device_settings_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_settings_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_get_device_settings_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_settings_request_token_t *token, const get_device_settings_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_device_settings_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_get_device_settings_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_get_device_settings_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_device_settings_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_device_settings_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_get_device_settings_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_get_motor_feedback_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  get_motor_feedback_response_t response;
} fci_arm_get_motor_feedback_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_call(fci_arm_endpoint_t *endpoint,
    const get_motor_feedback_request_t *request, uint32_t timeout_ms,
    fci_arm_get_motor_feedback_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_motor_feedback_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_get_motor_feedback_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_motor_feedback_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25107U && result->response_message_id == 25108U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_motor_feedback_call_t *call, fci_arm_get_motor_feedback_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_get_motor_feedback_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_get_motor_feedback_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_motor_feedback_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_get_motor_feedback_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_motor_feedback_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_get_motor_feedback_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_motor_feedback_request_token_t *token, const get_motor_feedback_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_motor_feedback_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_get_motor_feedback_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_get_motor_feedback_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_get_motor_feedback_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_get_motor_feedback_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_get_motor_feedback_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_home_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_home_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  home_response_t response;
} fci_arm_home_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_home_call(fci_arm_endpoint_t *endpoint,
    const home_request_t *request, uint32_t timeout_ms,
    fci_arm_home_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_home_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_home_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_home_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_home_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25089U && result->response_message_id == 25090U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_home_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_home_call_t *call, fci_arm_home_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_home_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_home_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_home_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_home_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_home_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_home_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_home_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_home_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_home_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_home_request_token_t *token, const home_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_home_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_home_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_home_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_home_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_home_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_home_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_motor_register_read_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  motor_register_read_response_t response;
} fci_arm_motor_register_read_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_call(fci_arm_endpoint_t *endpoint,
    const motor_register_read_request_t *request, uint32_t timeout_ms,
    fci_arm_motor_register_read_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_register_read_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_motor_register_read_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_read_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25117U && result->response_message_id == 25118U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_read_call_t *call, fci_arm_motor_register_read_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_motor_register_read_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_motor_register_read_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_read_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_register_read_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_read_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_register_read_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_read_request_token_t *token, const motor_register_read_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_register_read_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_motor_register_read_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_read_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_read_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_register_read_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_motor_register_read_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_motor_register_write_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  motor_register_write_response_t response;
} fci_arm_motor_register_write_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_call(fci_arm_endpoint_t *endpoint,
    const motor_register_write_request_t *request, uint32_t timeout_ms,
    fci_arm_motor_register_write_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_register_write_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_motor_register_write_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_write_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25119U && result->response_message_id == 25120U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_write_call_t *call, fci_arm_motor_register_write_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_motor_register_write_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_motor_register_write_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_write_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_register_write_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_write_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_register_write_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_write_request_token_t *token, const motor_register_write_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_register_write_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_motor_register_write_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_motor_register_write_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_register_write_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_register_write_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_motor_register_write_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_motor_set_zero_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  motor_set_zero_response_t response;
} fci_arm_motor_set_zero_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_call(fci_arm_endpoint_t *endpoint,
    const motor_set_zero_request_t *request, uint32_t timeout_ms,
    fci_arm_motor_set_zero_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_set_zero_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_motor_set_zero_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_set_zero_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25123U && result->response_message_id == 25124U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_set_zero_call_t *call, fci_arm_motor_set_zero_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_motor_set_zero_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_motor_set_zero_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_set_zero_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_set_zero_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_set_zero_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_set_zero_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_set_zero_request_token_t *token, const motor_set_zero_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_set_zero_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_motor_set_zero_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_motor_set_zero_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_set_zero_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_set_zero_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_motor_set_zero_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_motor_store_parameters_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  motor_store_parameters_response_t response;
} fci_arm_motor_store_parameters_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_call(fci_arm_endpoint_t *endpoint,
    const motor_store_parameters_request_t *request, uint32_t timeout_ms,
    fci_arm_motor_store_parameters_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_store_parameters_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_motor_store_parameters_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_store_parameters_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25121U && result->response_message_id == 25122U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_store_parameters_call_t *call, fci_arm_motor_store_parameters_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_motor_store_parameters_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_motor_store_parameters_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_store_parameters_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_store_parameters_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_store_parameters_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_motor_store_parameters_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_store_parameters_request_token_t *token, const motor_store_parameters_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_store_parameters_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_motor_store_parameters_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_motor_store_parameters_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_motor_store_parameters_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_motor_store_parameters_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_motor_store_parameters_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_release_control_lease_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  release_control_lease_response_t response;
} fci_arm_release_control_lease_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_call(fci_arm_endpoint_t *endpoint,
    const release_control_lease_request_t *request, uint32_t timeout_ms,
    fci_arm_release_control_lease_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_release_control_lease_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_release_control_lease_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_release_control_lease_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25099U && result->response_message_id == 25100U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_release_control_lease_call_t *call, fci_arm_release_control_lease_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_release_control_lease_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_release_control_lease_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_release_control_lease_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_release_control_lease_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_release_control_lease_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_release_control_lease_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_release_control_lease_request_token_t *token, const release_control_lease_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_release_control_lease_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_release_control_lease_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_release_control_lease_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_release_control_lease_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_release_control_lease_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_release_control_lease_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_set_arm_control_mode_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  set_arm_control_mode_response_t response;
} fci_arm_set_arm_control_mode_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_call(fci_arm_endpoint_t *endpoint,
    const set_arm_control_mode_request_t *request, uint32_t timeout_ms,
    fci_arm_set_arm_control_mode_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_arm_control_mode_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_set_arm_control_mode_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_control_mode_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25113U && result->response_message_id == 25114U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_control_mode_call_t *call, fci_arm_set_arm_control_mode_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_set_arm_control_mode_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_set_arm_control_mode_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_control_mode_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_arm_control_mode_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_control_mode_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_arm_control_mode_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_control_mode_request_token_t *token, const set_arm_control_mode_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_arm_control_mode_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_set_arm_control_mode_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_control_mode_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_control_mode_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_arm_control_mode_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_set_arm_control_mode_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_set_arm_mode_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  set_arm_mode_response_t response;
} fci_arm_set_arm_mode_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_call(fci_arm_endpoint_t *endpoint,
    const set_arm_mode_request_t *request, uint32_t timeout_ms,
    fci_arm_set_arm_mode_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_arm_mode_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_set_arm_mode_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_mode_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25125U && result->response_message_id == 25126U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_mode_call_t *call, fci_arm_set_arm_mode_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_set_arm_mode_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_set_arm_mode_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_mode_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_arm_mode_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_mode_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_arm_mode_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_mode_request_token_t *token, const set_arm_mode_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_arm_mode_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_set_arm_mode_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_set_arm_mode_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_arm_mode_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_arm_mode_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_set_arm_mode_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_set_device_info_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  set_device_info_response_t response;
} fci_arm_set_device_info_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_call(fci_arm_endpoint_t *endpoint,
    const set_device_info_request_t *request, uint32_t timeout_ms,
    fci_arm_set_device_info_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_device_info_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_set_device_info_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_info_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25111U && result->response_message_id == 25112U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_info_call_t *call, fci_arm_set_device_info_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_set_device_info_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_set_device_info_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_info_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_device_info_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_info_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_device_info_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_info_request_token_t *token, const set_device_info_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_device_info_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_set_device_info_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_set_device_info_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_info_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_device_info_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_set_device_info_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_set_device_settings_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  set_device_settings_response_t response;
} fci_arm_set_device_settings_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_call(fci_arm_endpoint_t *endpoint,
    const set_device_settings_request_t *request, uint32_t timeout_ms,
    fci_arm_set_device_settings_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_device_settings_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_set_device_settings_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_settings_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25130U && result->response_message_id == 25131U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_settings_call_t *call, fci_arm_set_device_settings_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_set_device_settings_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_set_device_settings_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_settings_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_device_settings_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_settings_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_device_settings_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_settings_request_token_t *token, const set_device_settings_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_device_settings_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_set_device_settings_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_set_device_settings_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_device_settings_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_device_settings_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_set_device_settings_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_set_gripper_control_mode_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  set_gripper_control_mode_response_t response;
} fci_arm_set_gripper_control_mode_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_call(fci_arm_endpoint_t *endpoint,
    const set_gripper_control_mode_request_t *request, uint32_t timeout_ms,
    fci_arm_set_gripper_control_mode_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_gripper_control_mode_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_set_gripper_control_mode_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_gripper_control_mode_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 25115U && result->response_message_id == 25116U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_gripper_control_mode_call_t *call, fci_arm_set_gripper_control_mode_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_set_gripper_control_mode_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_set_gripper_control_mode_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_gripper_control_mode_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_gripper_control_mode_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_gripper_control_mode_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_gripper_control_mode_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_gripper_control_mode_request_token_t *token, const set_gripper_control_mode_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_gripper_control_mode_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_set_gripper_control_mode_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_set_gripper_control_mode_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_gripper_control_mode_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_gripper_control_mode_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_set_gripper_control_mode_record_result(endpoint, &result);
}
#endif
/* Internal result bridge; detailed codec/link errors remain in endpoint_result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_zero_record_result(fci_arm_endpoint_t *endpoint,
    const fci_arm_runtime_result_t *result) {
  if (!endpoint->private_state.stepping ||
      fci_arm_runtime_result_ok(&endpoint->private_state.result))
    endpoint->private_state.result = *result;
  if (fci_arm_runtime_result_ok(result)) return WL_RPC_OK;
  return result->detail.rpc.rpc_result != WL_RPC_OK ? result->detail.rpc.rpc_result :
      (result->domain == FCI_ARM_RUNTIME_INVALID_ARGUMENT ? WL_RPC_ERR_INVALID_ARG : WL_RPC_ERR_INVALID_STATE);
}

#if FCI_ARM_RUNTIME_HAS_RPC_CLIENT
/* A copyable call handle, not a wire ID. Do not inspect private_state. Calls
 * belong to one endpoint incarnation; release only after a terminal result. */
typedef struct {
  struct {
    const fci_arm_endpoint_t *owner;
    uint64_t incarnation;
    wl_rpc_client_handle_t handle;
  } private_state;
} fci_arm_set_zero_call_t;

typedef struct {
  wl_rpc_client_state_t state;
  int32_t application_status;
  int32_t link_result;
  wl_rpc_err_t runtime_error;
  bool response_valid;
  /* Borrowed fields, if present in the schema, live until call release. */
  set_zero_response_t response;
} fci_arm_set_zero_result_t;

/* On success out_call identifies this invocation. On failure it is cleared;
 * endpoint_result retains detailed codec/link diagnostics. */
static inline wl_rpc_err_t fci_arm_endpoint_set_zero_call(fci_arm_endpoint_t *endpoint,
    const set_zero_request_t *request, uint32_t timeout_ms,
    fci_arm_set_zero_call_t *out_call) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_rpc_err_t error;
  wl_time_ms_t now_ms;
  if (out_call == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_call, 0, sizeof(*out_call));
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_zero_client_start(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)),
      runtime, request, timeout_ms, now_ms);
  error = fci_arm_endpoint_set_zero_record_result(endpoint, &result);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_get_handle(runtime->rpc_client,
      result.detail.rpc.operation_id, &out_call->private_state.handle);
  if (error != WL_RPC_OK) return error;
  out_call->private_state.owner = endpoint;
  out_call->private_state.incarnation = endpoint->private_state.incarnation;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_zero_call_get(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_zero_call_t *call, wl_rpc_client_result_t *result) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  wl_rpc_err_t error;
  if (call == NULL || result == NULL) return WL_RPC_ERR_INVALID_ARG;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  if (call->private_state.owner != endpoint ||
      call->private_state.incarnation != endpoint->private_state.incarnation)
    return WL_RPC_ERR_NOT_FOUND;
  error = wl_rpc_client_get_by_handle(runtime->rpc_client, &call->private_state.handle, result);
  if (error != WL_RPC_OK) return error;
  return result->request_message_id == 24839U && result->response_message_id == 24840U
      ? WL_RPC_OK : WL_RPC_ERR_RESPONSE_MISMATCH;
}

/* Pending and terminal states both return RPC_OK. A rejection has a nonzero
 * application_status and no response body; it is not a decode failure. */
static inline wl_rpc_err_t fci_arm_endpoint_set_zero_inspect(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_zero_call_t *call, fci_arm_set_zero_result_t *out_result) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error;
  fci_arm_runtime_result_t decoded;
  if (out_result == NULL) return WL_RPC_ERR_INVALID_ARG;
  memset(out_result, 0, sizeof(*out_result));
  error = fci_arm_endpoint_set_zero_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  out_result->state = client.state;
  out_result->application_status = client.application_status;
  out_result->link_result = client.link_result;
  out_result->runtime_error = client.runtime_error;
  if (client.state != WL_RPC_CLIENT_COMPLETED) return WL_RPC_OK;
  decoded = fci_arm_set_zero_client_decode(&client, &out_result->response);
  if (!fci_arm_runtime_result_ok(&decoded)) {
    endpoint->private_state.result = decoded;
    return decoded.detail.rpc.rpc_result != WL_RPC_OK ? decoded.detail.rpc.rpc_result : WL_RPC_ERR_INVALID_STATE;
  }
  out_result->response_valid = true;
  return WL_RPC_OK;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_zero_release(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_zero_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_zero_call_get(endpoint, call, &client);
  return error == WL_RPC_OK ? wl_rpc_client_release_handle(
      fci_arm_endpoint_runtime(endpoint)->rpc_client, &call->private_state.handle) : error;
}

static inline wl_rpc_err_t fci_arm_endpoint_set_zero_cancel(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_zero_call_t *call) {
  wl_rpc_client_result_t client;
  wl_rpc_err_t error = fci_arm_endpoint_set_zero_call_get(endpoint, call, &client);
  if (error != WL_RPC_OK) return error;
  error = wl_rpc_client_cancel_handle(fci_arm_endpoint_runtime(endpoint)->rpc_client,
      &call->private_state.handle);
  if (error == WL_RPC_OK && client.tx_handle != 0U)
    (void)wl_tx_cancel(wl_endpoint_link(fci_arm_endpoint_handle(endpoint)), client.tx_handle);
  return error;
}

#endif

#if FCI_ARM_RUNTIME_HAS_RPC_SERVER
/* Reply submission returns an RPC error code, not a generic runtime result. */
static inline wl_rpc_err_t fci_arm_endpoint_set_zero_complete(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_zero_request_token_t *token, const set_zero_response_t *response) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_zero_server_complete(runtime, token, response, now_ms);
  return fci_arm_endpoint_set_zero_record_result(endpoint, &result);
}
static inline wl_rpc_err_t fci_arm_endpoint_set_zero_reject(fci_arm_endpoint_t *endpoint,
    const fci_arm_set_zero_request_token_t *token, int32_t application_status) {
  fci_arm_runtime_t *runtime = fci_arm_endpoint_runtime(endpoint);
  fci_arm_runtime_result_t result;
  wl_time_ms_t now_ms;
  if (runtime == NULL) return WL_RPC_ERR_NOT_INITIALIZED;
  (void)wl_endpoint_now(fci_arm_endpoint_handle(endpoint), &now_ms);
  result = fci_arm_set_zero_server_reject(runtime, token, application_status, now_ms);
  return fci_arm_endpoint_set_zero_record_result(endpoint, &result);
}
#endif
#endif
#endif
