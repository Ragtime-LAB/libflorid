#include "fci_device_bindings.h"

#include <limits.h>

static void fci_device_count(uint32_t *counter) {
  if (*counter != UINT32_MAX) ++*counter;
}

fci_device_dispatch_result_t fci_device_dispatch_event(wl_ctx_t *ctx, const wl_event_t *event, fci_device_router_t *router) {
  fci_device_dispatch_result_t result = { FCI_DEVICE_DISPATCH_INVALID_ARGUMENT, 0U, WL_EVT_NONE, WL_CODEC_OK, 0 };
  if (event == NULL) return result;
  result.message_id = event->message_id;
  result.event_type = event->type;
  if (event->type != WL_EVT_UNRELIABLE_RX && event->type != WL_EVT_RELIABLE_RX) {
    result.domain = FCI_DEVICE_DISPATCH_NON_RX;
    if (router != NULL) fci_device_count(&router->counters.non_rx);
    return result;
  }
  if (ctx == NULL) return result;
  switch (event->message_id) {
    case SEMANTIC_VERSION_MESSAGE_ID:
      if (router == NULL || router->semantic_version.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->semantic_version.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = semantic_version_decode(event->payload, event->payload_len, router->semantic_version.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->semantic_version.handler(router->semantic_version.user_data, router->semantic_version.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case DEVICE_INFO_MESSAGE_ID:
      if (router == NULL || router->device_info.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->device_info.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = device_info_decode(event->payload, event->payload_len, router->device_info.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->device_info.handler(router->device_info.user_data, router->device_info.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case DEVICE_SETTINGS_MESSAGE_ID:
      if (router == NULL || router->device_settings.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->device_settings.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = device_settings_decode(event->payload, event->payload_len, router->device_settings.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->device_settings.handler(router->device_settings.user_data, router->device_settings.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case ARM_STATUS_MESSAGE_ID:
      if (router == NULL || router->arm_status.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->arm_status.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = arm_status_decode(event->payload, event->payload_len, router->arm_status.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->arm_status.handler(router->arm_status.user_data, router->arm_status.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_FEEDBACK_MESSAGE_ID:
      if (router == NULL || router->motor_feedback.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_feedback.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_feedback_decode(event->payload, event->payload_len, router->motor_feedback.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_feedback.handler(router->motor_feedback.user_data, router->motor_feedback.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case ARM_DIAGNOSTICS_MESSAGE_ID:
      if (router == NULL || router->arm_diagnostics.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->arm_diagnostics.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = arm_diagnostics_decode(event->payload, event->payload_len, router->arm_diagnostics.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->arm_diagnostics.handler(router->arm_diagnostics.user_data, router->arm_diagnostics.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_ZERO_REQUEST_MESSAGE_ID:
      if (router == NULL || router->set_zero_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_zero_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_zero_request_decode(event->payload, event->payload_len, router->set_zero_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_zero_request.handler(router->set_zero_request.user_data, router->set_zero_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_ZERO_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->set_zero_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_zero_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_zero_response_decode(event->payload, event->payload_len, router->set_zero_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_zero_response.handler(router->set_zero_response.user_data, router->set_zero_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case CLEAR_ERROR_REQUEST_MESSAGE_ID:
      if (router == NULL || router->clear_error_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->clear_error_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = clear_error_request_decode(event->payload, event->payload_len, router->clear_error_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->clear_error_request.handler(router->clear_error_request.user_data, router->clear_error_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case CLEAR_ERROR_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->clear_error_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->clear_error_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = clear_error_response_decode(event->payload, event->payload_len, router->clear_error_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->clear_error_response.handler(router->clear_error_response.user_data, router->clear_error_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case HOME_REQUEST_MESSAGE_ID:
      if (router == NULL || router->home_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->home_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = home_request_decode(event->payload, event->payload_len, router->home_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->home_request.handler(router->home_request.user_data, router->home_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case HOME_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->home_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->home_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = home_response_decode(event->payload, event->payload_len, router->home_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->home_response.handler(router->home_response.user_data, router->home_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case CLEAR_FAULTS_REQUEST_MESSAGE_ID:
      if (router == NULL || router->clear_faults_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->clear_faults_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = clear_faults_request_decode(event->payload, event->payload_len, router->clear_faults_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->clear_faults_request.handler(router->clear_faults_request.user_data, router->clear_faults_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case CLEAR_FAULTS_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->clear_faults_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->clear_faults_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = clear_faults_response_decode(event->payload, event->payload_len, router->clear_faults_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->clear_faults_response.handler(router->clear_faults_response.user_data, router->clear_faults_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->acquire_control_lease_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->acquire_control_lease_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = acquire_control_lease_request_decode(event->payload, event->payload_len, router->acquire_control_lease_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->acquire_control_lease_request.handler(router->acquire_control_lease_request.user_data, router->acquire_control_lease_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->acquire_control_lease_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->acquire_control_lease_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = acquire_control_lease_response_decode(event->payload, event->payload_len, router->acquire_control_lease_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->acquire_control_lease_response.handler(router->acquire_control_lease_response.user_data, router->acquire_control_lease_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->release_control_lease_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->release_control_lease_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = release_control_lease_request_decode(event->payload, event->payload_len, router->release_control_lease_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->release_control_lease_request.handler(router->release_control_lease_request.user_data, router->release_control_lease_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->release_control_lease_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->release_control_lease_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = release_control_lease_response_decode(event->payload, event->payload_len, router->release_control_lease_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->release_control_lease_response.handler(router->release_control_lease_response.user_data, router->release_control_lease_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID:
      if (router == NULL || router->get_motor_feedback_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_motor_feedback_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_motor_feedback_request_decode(event->payload, event->payload_len, router->get_motor_feedback_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_motor_feedback_request.handler(router->get_motor_feedback_request.user_data, router->get_motor_feedback_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->get_motor_feedback_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_motor_feedback_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_motor_feedback_response_decode(event->payload, event->payload_len, router->get_motor_feedback_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_motor_feedback_response.handler(router->get_motor_feedback_response.user_data, router->get_motor_feedback_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_DEVICE_INFO_REQUEST_MESSAGE_ID:
      if (router == NULL || router->get_device_info_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_device_info_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_device_info_request_decode(event->payload, event->payload_len, router->get_device_info_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_device_info_request.handler(router->get_device_info_request.user_data, router->get_device_info_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_DEVICE_INFO_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->get_device_info_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_device_info_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_device_info_response_decode(event->payload, event->payload_len, router->get_device_info_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_device_info_response.handler(router->get_device_info_response.user_data, router->get_device_info_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_DEVICE_INFO_REQUEST_MESSAGE_ID:
      if (router == NULL || router->set_device_info_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_device_info_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_device_info_request_decode(event->payload, event->payload_len, router->set_device_info_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_device_info_request.handler(router->set_device_info_request.user_data, router->set_device_info_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_DEVICE_INFO_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->set_device_info_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_device_info_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_device_info_response_decode(event->payload, event->payload_len, router->set_device_info_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_device_info_response.handler(router->set_device_info_response.user_data, router->set_device_info_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->set_arm_control_mode_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_arm_control_mode_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_arm_control_mode_request_decode(event->payload, event->payload_len, router->set_arm_control_mode_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_arm_control_mode_request.handler(router->set_arm_control_mode_request.user_data, router->set_arm_control_mode_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->set_arm_control_mode_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_arm_control_mode_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_arm_control_mode_response_decode(event->payload, event->payload_len, router->set_arm_control_mode_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_arm_control_mode_response.handler(router->set_arm_control_mode_response.user_data, router->set_arm_control_mode_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->set_gripper_control_mode_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_gripper_control_mode_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_gripper_control_mode_request_decode(event->payload, event->payload_len, router->set_gripper_control_mode_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_gripper_control_mode_request.handler(router->set_gripper_control_mode_request.user_data, router->set_gripper_control_mode_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->set_gripper_control_mode_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_gripper_control_mode_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_gripper_control_mode_response_decode(event->payload, event->payload_len, router->set_gripper_control_mode_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_gripper_control_mode_response.handler(router->set_gripper_control_mode_response.user_data, router->set_gripper_control_mode_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID:
      if (router == NULL || router->motor_register_read_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_register_read_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_register_read_request_decode(event->payload, event->payload_len, router->motor_register_read_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_register_read_request.handler(router->motor_register_read_request.user_data, router->motor_register_read_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->motor_register_read_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_register_read_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_register_read_response_decode(event->payload, event->payload_len, router->motor_register_read_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_register_read_response.handler(router->motor_register_read_response.user_data, router->motor_register_read_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->motor_register_write_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_register_write_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_register_write_request_decode(event->payload, event->payload_len, router->motor_register_write_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_register_write_request.handler(router->motor_register_write_request.user_data, router->motor_register_write_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->motor_register_write_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_register_write_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_register_write_response_decode(event->payload, event->payload_len, router->motor_register_write_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_register_write_response.handler(router->motor_register_write_response.user_data, router->motor_register_write_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID:
      if (router == NULL || router->motor_store_parameters_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_store_parameters_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_store_parameters_request_decode(event->payload, event->payload_len, router->motor_store_parameters_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_store_parameters_request.handler(router->motor_store_parameters_request.user_data, router->motor_store_parameters_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->motor_store_parameters_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_store_parameters_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_store_parameters_response_decode(event->payload, event->payload_len, router->motor_store_parameters_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_store_parameters_response.handler(router->motor_store_parameters_response.user_data, router->motor_store_parameters_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_SET_ZERO_REQUEST_MESSAGE_ID:
      if (router == NULL || router->motor_set_zero_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_set_zero_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_set_zero_request_decode(event->payload, event->payload_len, router->motor_set_zero_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_set_zero_request.handler(router->motor_set_zero_request.user_data, router->motor_set_zero_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->motor_set_zero_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->motor_set_zero_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = motor_set_zero_response_decode(event->payload, event->payload_len, router->motor_set_zero_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->motor_set_zero_response.handler(router->motor_set_zero_response.user_data, router->motor_set_zero_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_ARM_MODE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->set_arm_mode_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_arm_mode_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_arm_mode_request_decode(event->payload, event->payload_len, router->set_arm_mode_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_arm_mode_request.handler(router->set_arm_mode_request.user_data, router->set_arm_mode_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_ARM_MODE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->set_arm_mode_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_arm_mode_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_arm_mode_response_decode(event->payload, event->payload_len, router->set_arm_mode_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_arm_mode_response.handler(router->set_arm_mode_response.user_data, router->set_arm_mode_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID:
      if (router == NULL || router->get_device_settings_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_device_settings_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_device_settings_request_decode(event->payload, event->payload_len, router->get_device_settings_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_device_settings_request.handler(router->get_device_settings_request.user_data, router->get_device_settings_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->get_device_settings_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_device_settings_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_device_settings_response_decode(event->payload, event->payload_len, router->get_device_settings_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_device_settings_response.handler(router->get_device_settings_response.user_data, router->get_device_settings_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID:
      if (router == NULL || router->set_device_settings_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_device_settings_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_device_settings_request_decode(event->payload, event->payload_len, router->set_device_settings_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_device_settings_request.handler(router->set_device_settings_request.user_data, router->set_device_settings_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->set_device_settings_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->set_device_settings_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = set_device_settings_response_decode(event->payload, event->payload_len, router->set_device_settings_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->set_device_settings_response.handler(router->set_device_settings_response.user_data, router->set_device_settings_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case JOINT_MIT_COMMAND_MESSAGE_ID:
      if (router == NULL || router->joint_mit_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->joint_mit_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = joint_mit_command_decode(event->payload, event->payload_len, router->joint_mit_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->joint_mit_command.handler(router->joint_mit_command.user_data, router->joint_mit_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case EMERGENCY_STOP_REQUEST_MESSAGE_ID:
      if (router == NULL || router->emergency_stop_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->emergency_stop_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = emergency_stop_request_decode(event->payload, event->payload_len, router->emergency_stop_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->emergency_stop_request.handler(router->emergency_stop_request.user_data, router->emergency_stop_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case EMERGENCY_STOP_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->emergency_stop_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->emergency_stop_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = emergency_stop_response_decode(event->payload, event->payload_len, router->emergency_stop_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->emergency_stop_response.handler(router->emergency_stop_response.user_data, router->emergency_stop_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GRIPPER_MIT_COMMAND_MESSAGE_ID:
      if (router == NULL || router->gripper_mit_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->gripper_mit_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = gripper_mit_command_decode(event->payload, event->payload_len, router->gripper_mit_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->gripper_mit_command.handler(router->gripper_mit_command.user_data, router->gripper_mit_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case JOINT_POSITION_VELOCITY_COMMAND_MESSAGE_ID:
      if (router == NULL || router->joint_position_velocity_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->joint_position_velocity_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = joint_position_velocity_command_decode(event->payload, event->payload_len, router->joint_position_velocity_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->joint_position_velocity_command.handler(router->joint_position_velocity_command.user_data, router->joint_position_velocity_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case JOINT_VELOCITY_COMMAND_MESSAGE_ID:
      if (router == NULL || router->joint_velocity_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->joint_velocity_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = joint_velocity_command_decode(event->payload, event->payload_len, router->joint_velocity_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->joint_velocity_command.handler(router->joint_velocity_command.user_data, router->joint_velocity_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case JOINT_PVT_COMMAND_MESSAGE_ID:
      if (router == NULL || router->joint_pvt_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->joint_pvt_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = joint_pvt_command_decode(event->payload, event->payload_len, router->joint_pvt_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->joint_pvt_command.handler(router->joint_pvt_command.user_data, router->joint_pvt_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case CARTESIAN_POSE_COMMAND_MESSAGE_ID:
      if (router == NULL || router->cartesian_pose_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->cartesian_pose_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = cartesian_pose_command_decode(event->payload, event->payload_len, router->cartesian_pose_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->cartesian_pose_command.handler(router->cartesian_pose_command.user_data, router->cartesian_pose_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case CARTESIAN_VELOCITY_COMMAND_MESSAGE_ID:
      if (router == NULL || router->cartesian_velocity_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->cartesian_velocity_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = cartesian_velocity_command_decode(event->payload, event->payload_len, router->cartesian_velocity_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->cartesian_velocity_command.handler(router->cartesian_velocity_command.user_data, router->cartesian_velocity_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID:
      if (router == NULL || router->gripper_position_velocity_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->gripper_position_velocity_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = gripper_position_velocity_command_decode(event->payload, event->payload_len, router->gripper_position_velocity_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->gripper_position_velocity_command.handler(router->gripper_position_velocity_command.user_data, router->gripper_position_velocity_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GRIPPER_VELOCITY_COMMAND_MESSAGE_ID:
      if (router == NULL || router->gripper_velocity_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->gripper_velocity_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = gripper_velocity_command_decode(event->payload, event->payload_len, router->gripper_velocity_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->gripper_velocity_command.handler(router->gripper_velocity_command.user_data, router->gripper_velocity_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GRIPPER_PVT_COMMAND_MESSAGE_ID:
      if (router == NULL || router->gripper_pvt_command.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->gripper_pvt_command.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = gripper_pvt_command_decode(event->payload, event->payload_len, router->gripper_pvt_command.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->gripper_pvt_command.handler(router->gripper_pvt_command.user_data, router->gripper_pvt_command.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case IMAGE_VERSION_MESSAGE_ID:
      if (router == NULL || router->image_version.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->image_version.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = image_version_decode(event->payload, event->payload_len, router->image_version.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->image_version.handler(router->image_version.user_data, router->image_version.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case BOOT_STATUS_INFO_MESSAGE_ID:
      if (router == NULL || router->boot_status_info.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->boot_status_info.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = boot_status_info_decode(event->payload, event->payload_len, router->boot_status_info.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->boot_status_info.handler(router->boot_status_info.user_data, router->boot_status_info.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case BULK_BEGIN_MESSAGE_ID:
      if (router == NULL || router->bulk_begin.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->bulk_begin.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = bulk_begin_decode(event->payload, event->payload_len, router->bulk_begin.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->bulk_begin.handler(router->bulk_begin.user_data, router->bulk_begin.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case BULK_CHUNK_MESSAGE_ID:
      if (router == NULL || router->bulk_chunk.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->bulk_chunk.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = bulk_chunk_decode(event->payload, event->payload_len, router->bulk_chunk.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->bulk_chunk.handler(router->bulk_chunk.user_data, router->bulk_chunk.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case BULK_END_MESSAGE_ID:
      if (router == NULL || router->bulk_end.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->bulk_end.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = bulk_end_decode(event->payload, event->payload_len, router->bulk_end.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->bulk_end.handler(router->bulk_end.user_data, router->bulk_end.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case BULK_ABORT_MESSAGE_ID:
      if (router == NULL || router->bulk_abort.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->bulk_abort.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = bulk_abort_decode(event->payload, event->payload_len, router->bulk_abort.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->bulk_abort.handler(router->bulk_abort.user_data, router->bulk_abort.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case BULK_STATUS_MESSAGE_ID:
      if (router == NULL || router->bulk_status.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->bulk_status.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = bulk_status_decode(event->payload, event->payload_len, router->bulk_status.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->bulk_status.handler(router->bulk_status.user_data, router->bulk_status.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_BOOT_STATUS_REQUEST_MESSAGE_ID:
      if (router == NULL || router->get_boot_status_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_boot_status_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_boot_status_request_decode(event->payload, event->payload_len, router->get_boot_status_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_boot_status_request.handler(router->get_boot_status_request.user_data, router->get_boot_status_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case GET_BOOT_STATUS_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->get_boot_status_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->get_boot_status_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = get_boot_status_response_decode(event->payload, event->payload_len, router->get_boot_status_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->get_boot_status_response.handler(router->get_boot_status_response.user_data, router->get_boot_status_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case START_UPGRADE_REQUEST_MESSAGE_ID:
      if (router == NULL || router->start_upgrade_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->start_upgrade_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = start_upgrade_request_decode(event->payload, event->payload_len, router->start_upgrade_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->start_upgrade_request.handler(router->start_upgrade_request.user_data, router->start_upgrade_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case START_UPGRADE_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->start_upgrade_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->start_upgrade_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = start_upgrade_response_decode(event->payload, event->payload_len, router->start_upgrade_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->start_upgrade_response.handler(router->start_upgrade_response.user_data, router->start_upgrade_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case REBOOT_REQUEST_MESSAGE_ID:
      if (router == NULL || router->reboot_request.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->reboot_request.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = reboot_request_decode(event->payload, event->payload_len, router->reboot_request.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->reboot_request.handler(router->reboot_request.user_data, router->reboot_request.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    case REBOOT_RESPONSE_MESSAGE_ID:
      if (router == NULL || router->reboot_response.handler == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_ROUTE;
        if (router != NULL) fci_device_count(&router->counters.missing_route);
        break;
      }
      if (router->reboot_response.scratch == NULL) {
        result.domain = FCI_DEVICE_DISPATCH_MISSING_SCRATCH;
        fci_device_count(&router->counters.missing_scratch);
        break;
      }
      result.codec_status = reboot_response_decode(event->payload, event->payload_len, router->reboot_response.scratch);
      if (result.codec_status != WL_CODEC_OK) {
        result.domain = FCI_DEVICE_DISPATCH_CODEC_ERROR;
        fci_device_count(&router->counters.codec_failure);
        break;
      }
      result.handler_result = router->reboot_response.handler(router->reboot_response.user_data, router->reboot_response.scratch, event->type == WL_EVT_RELIABLE_RX ? WL_DELIVERY_RELIABLE : WL_DELIVERY_UNRELIABLE);
      if (result.handler_result != 0) {
        result.domain = FCI_DEVICE_DISPATCH_HANDLER_ERROR;
        fci_device_count(&router->counters.handler_failure);
        break;
      }
      result.domain = FCI_DEVICE_DISPATCH_OK;
      fci_device_count(&router->counters.delivered);
      break;
    default:
      result.domain = FCI_DEVICE_DISPATCH_UNKNOWN_MESSAGE;
      if (router != NULL) fci_device_count(&router->counters.unknown_message);
      break;
  }
  wl_event_release(ctx, event);
  return result;
}

fci_device_send_result_t fci_device_semantic_version_send(wl_ctx_t *ctx, const semantic_version_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SEMANTIC_VERSION_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = semantic_version_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_device_info_send(wl_ctx_t *ctx, const device_info_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, DEVICE_INFO_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = device_info_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_device_settings_send(wl_ctx_t *ctx, const device_settings_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, DEVICE_SETTINGS_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = device_settings_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_arm_status_send(wl_ctx_t *ctx, const arm_status_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, ARM_STATUS_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = arm_status_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_feedback_send(wl_ctx_t *ctx, const motor_feedback_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_FEEDBACK_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_feedback_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_arm_diagnostics_send(wl_ctx_t *ctx, const arm_diagnostics_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, ARM_DIAGNOSTICS_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = arm_diagnostics_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_zero_request_send(wl_ctx_t *ctx, const set_zero_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_ZERO_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_zero_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_zero_response_send(wl_ctx_t *ctx, const set_zero_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_ZERO_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_zero_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_clear_error_request_send(wl_ctx_t *ctx, const clear_error_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, CLEAR_ERROR_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = clear_error_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_clear_error_response_send(wl_ctx_t *ctx, const clear_error_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, CLEAR_ERROR_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = clear_error_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_home_request_send(wl_ctx_t *ctx, const home_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, HOME_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = home_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_home_response_send(wl_ctx_t *ctx, const home_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, HOME_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = home_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_clear_faults_request_send(wl_ctx_t *ctx, const clear_faults_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, CLEAR_FAULTS_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = clear_faults_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_clear_faults_response_send(wl_ctx_t *ctx, const clear_faults_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, CLEAR_FAULTS_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = clear_faults_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_acquire_control_lease_request_send(wl_ctx_t *ctx, const acquire_control_lease_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = acquire_control_lease_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_acquire_control_lease_response_send(wl_ctx_t *ctx, const acquire_control_lease_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = acquire_control_lease_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_release_control_lease_request_send(wl_ctx_t *ctx, const release_control_lease_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = release_control_lease_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_release_control_lease_response_send(wl_ctx_t *ctx, const release_control_lease_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = release_control_lease_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_motor_feedback_request_send(wl_ctx_t *ctx, const get_motor_feedback_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_motor_feedback_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_motor_feedback_response_send(wl_ctx_t *ctx, const get_motor_feedback_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_motor_feedback_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_device_info_request_send(wl_ctx_t *ctx, const get_device_info_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_DEVICE_INFO_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_device_info_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_device_info_response_send(wl_ctx_t *ctx, const get_device_info_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_DEVICE_INFO_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_device_info_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_device_info_request_send(wl_ctx_t *ctx, const set_device_info_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_DEVICE_INFO_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_device_info_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_device_info_response_send(wl_ctx_t *ctx, const set_device_info_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_DEVICE_INFO_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_device_info_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_arm_control_mode_request_send(wl_ctx_t *ctx, const set_arm_control_mode_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_ARM_CONTROL_MODE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_arm_control_mode_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_arm_control_mode_response_send(wl_ctx_t *ctx, const set_arm_control_mode_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_ARM_CONTROL_MODE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_arm_control_mode_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_gripper_control_mode_request_send(wl_ctx_t *ctx, const set_gripper_control_mode_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_GRIPPER_CONTROL_MODE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_gripper_control_mode_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_gripper_control_mode_response_send(wl_ctx_t *ctx, const set_gripper_control_mode_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_GRIPPER_CONTROL_MODE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_gripper_control_mode_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_register_read_request_send(wl_ctx_t *ctx, const motor_register_read_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_REGISTER_READ_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_register_read_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_register_read_response_send(wl_ctx_t *ctx, const motor_register_read_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_REGISTER_READ_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_register_read_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_register_write_request_send(wl_ctx_t *ctx, const motor_register_write_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_REGISTER_WRITE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_register_write_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_register_write_response_send(wl_ctx_t *ctx, const motor_register_write_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_REGISTER_WRITE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_register_write_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_store_parameters_request_send(wl_ctx_t *ctx, const motor_store_parameters_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_STORE_PARAMETERS_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_store_parameters_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_store_parameters_response_send(wl_ctx_t *ctx, const motor_store_parameters_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_STORE_PARAMETERS_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_store_parameters_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_set_zero_request_send(wl_ctx_t *ctx, const motor_set_zero_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_SET_ZERO_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_set_zero_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_motor_set_zero_response_send(wl_ctx_t *ctx, const motor_set_zero_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, MOTOR_SET_ZERO_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = motor_set_zero_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_arm_mode_request_send(wl_ctx_t *ctx, const set_arm_mode_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_ARM_MODE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_arm_mode_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_arm_mode_response_send(wl_ctx_t *ctx, const set_arm_mode_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_ARM_MODE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_arm_mode_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_device_settings_request_send(wl_ctx_t *ctx, const get_device_settings_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_device_settings_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_device_settings_response_send(wl_ctx_t *ctx, const get_device_settings_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_device_settings_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_device_settings_request_send(wl_ctx_t *ctx, const set_device_settings_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_DEVICE_SETTINGS_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_device_settings_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_set_device_settings_response_send(wl_ctx_t *ctx, const set_device_settings_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, SET_DEVICE_SETTINGS_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = set_device_settings_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_joint_mit_command_send(wl_ctx_t *ctx, const joint_mit_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, JOINT_MIT_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = joint_mit_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_emergency_stop_request_send(wl_ctx_t *ctx, const emergency_stop_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, EMERGENCY_STOP_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = emergency_stop_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_emergency_stop_response_send(wl_ctx_t *ctx, const emergency_stop_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, EMERGENCY_STOP_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = emergency_stop_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_gripper_mit_command_send(wl_ctx_t *ctx, const gripper_mit_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GRIPPER_MIT_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = gripper_mit_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_joint_position_velocity_command_send(wl_ctx_t *ctx, const joint_position_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, JOINT_POSITION_VELOCITY_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = joint_position_velocity_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_joint_velocity_command_send(wl_ctx_t *ctx, const joint_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, JOINT_VELOCITY_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = joint_velocity_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_joint_pvt_command_send(wl_ctx_t *ctx, const joint_pvt_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, JOINT_PVT_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = joint_pvt_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_cartesian_pose_command_send(wl_ctx_t *ctx, const cartesian_pose_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, CARTESIAN_POSE_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = cartesian_pose_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_cartesian_velocity_command_send(wl_ctx_t *ctx, const cartesian_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, CARTESIAN_VELOCITY_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = cartesian_velocity_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_gripper_position_velocity_command_send(wl_ctx_t *ctx, const gripper_position_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GRIPPER_POSITION_VELOCITY_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = gripper_position_velocity_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_gripper_velocity_command_send(wl_ctx_t *ctx, const gripper_velocity_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GRIPPER_VELOCITY_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = gripper_velocity_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_gripper_pvt_command_send(wl_ctx_t *ctx, const gripper_pvt_command_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GRIPPER_PVT_COMMAND_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = gripper_pvt_command_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_image_version_send(wl_ctx_t *ctx, const image_version_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, IMAGE_VERSION_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = image_version_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_boot_status_info_send(wl_ctx_t *ctx, const boot_status_info_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, BOOT_STATUS_INFO_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = boot_status_info_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_bulk_begin_send(wl_ctx_t *ctx, const bulk_begin_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, BULK_BEGIN_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = bulk_begin_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_bulk_chunk_send(wl_ctx_t *ctx, const bulk_chunk_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, BULK_CHUNK_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = bulk_chunk_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_bulk_end_send(wl_ctx_t *ctx, const bulk_end_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, BULK_END_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = bulk_end_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_bulk_abort_send(wl_ctx_t *ctx, const bulk_abort_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, BULK_ABORT_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = bulk_abort_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_bulk_status_send(wl_ctx_t *ctx, const bulk_status_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, BULK_STATUS_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = bulk_status_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_boot_status_request_send(wl_ctx_t *ctx, const get_boot_status_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_BOOT_STATUS_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_boot_status_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_get_boot_status_response_send(wl_ctx_t *ctx, const get_boot_status_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, GET_BOOT_STATUS_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = get_boot_status_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_start_upgrade_request_send(wl_ctx_t *ctx, const start_upgrade_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, START_UPGRADE_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = start_upgrade_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_start_upgrade_response_send(wl_ctx_t *ctx, const start_upgrade_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, START_UPGRADE_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = start_upgrade_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_reboot_request_send(wl_ctx_t *ctx, const reboot_request_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, REBOOT_REQUEST_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = reboot_request_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}

fci_device_send_result_t fci_device_reboot_response_send(wl_ctx_t *ctx, const reboot_response_t *message, wl_delivery_t delivery, wl_time_ms_t now_ms) {
  fci_device_send_result_t result = { FCI_DEVICE_SEND_CORE_ERROR, WL_CODEC_OK, WL_OK, 0U, 0U };
  wl_tx_payload_claim_t claim = {0};
  result.core_result = wl_tx_payload_claim(ctx, REBOOT_RESPONSE_MESSAGE_ID, delivery, &claim);
  if (result.core_result != WL_OK) return result;
  result.codec_status = reboot_response_encode(message, claim.span.data, claim.span.length, &result.payload_length);
  if (result.codec_status != WL_CODEC_OK) {
    result.domain = FCI_DEVICE_SEND_CODEC_ERROR;
    (void)wl_tx_payload_abort(ctx, &claim);
    return result;
  }
  result.core_result = wl_tx_payload_commit(ctx, &claim, result.payload_length, now_ms, delivery == WL_DELIVERY_RELIABLE ? &result.handle : NULL);
  if (result.core_result != WL_OK) return result;
  result.domain = FCI_DEVICE_SEND_OK;
  return result;
}
