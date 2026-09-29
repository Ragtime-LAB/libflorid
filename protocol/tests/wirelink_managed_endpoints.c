/* SPDX-License-Identifier: ISC */
#include "fci_arm_host_endpoint.h"
#include "fci_arm_firmware_endpoint.h"
#include "wirelink/port.h"
#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
static fci_arm_host_endpoint_t host;
static fci_arm_firmware_endpoint_t firmware;
static wl_time_ms_t now = 100U;
static unsigned completed, served;
static wl_rpc_status_t last_status;
static int32_t last_rejection;
static get_device_info_response_value_t saved;
static char name[32];

static wl_time_ms_t clock_now(void *context) { (void)context; return now; }
static wl_err_t session(void *context, uint64_t *out) {
    *out = *(const uint64_t *)context;
    return WL_OK;
}
/* Same-owner COBS transport; the runtime, not this sink, dispatches input. */
static wl_sink_result_t sink(void *peer, wl_io_token_t token, const uint8_t *data, size_t length) {
    size_t accepted = 0U;
    (void)token;
    return wl_feed_bytes(wl_endpoint_link(peer), data, length, &accepted) == WL_OK && accepted == length
        ? WL_SINK_SENT : WL_SINK_FAILED;
}
static int32_t home_handler(void *context, const home_request_value_t *request,
                           home_response_value_t *response) {
    (void)context; (void)request; (void)response;
    ++served;
    return 0;
}
static int32_t clear_handler(void *context, const clear_faults_request_value_t *request,
                            clear_faults_response_value_t *response) {
    (void)context; (void)request; (void)response;
    return FAULT_OPERATION_UNSUPPORTED;
}
static int32_t rename_handler(void *context, const set_device_info_request_value_t *request,
                             set_device_info_response_value_t *response) {
    (void)context; (void)response;
    memcpy(name, request->custom_name.data, request->custom_name.length);
    name[request->custom_name.length] = '\0';
    return 0;
}
static int32_t info_handler(void *context, const get_device_info_request_value_t *request,
                           get_device_info_response_value_t *response) {
    (void)context; (void)request;
    response->has_info = true;
    device_info_value_t *info = &response->info;
    info->has_protocol_version = info->has_firmware_version = true;
    info->protocol_version.has_major = info->protocol_version.has_minor = info->protocol_version.has_patch = true;
    info->firmware_version = info->protocol_version;
    info->has_board_name = info->has_custom_name = info->has_serial = info->has_firmware_type = true;
    info->board_name.length = info->serial.length = 1U;
    info->board_name.data[0] = 'B'; info->serial.data[0] = 'S';
    info->custom_name.length = strlen(name);
    memcpy(info->custom_name.data, name, info->custom_name.length);
    return 0;
}
#define COMPLETION(service) \
static void service##_done(void *context, const wl_rpc_completion_t *result, \
                           const service##_response_value_t *response) { \
    (void)context; (void)response; ++completed; \
    last_status = result->status; last_rejection = result->rejection; \
}
COMPLETION(home)
COMPLETION(clear_faults)
COMPLETION(set_device_info)
#undef COMPLETION
static void info_done(void *context, const wl_rpc_completion_t *result,
                      const get_device_info_response_value_t *response) {
    (void)context; ++completed; last_status = result->status;
    if (response != NULL) saved = *response;
}
static int drive(unsigned expected) {
    for (unsigned i = 0U; i < 16U && completed < expected; ++i, ++now) {
        CHECK(fci_arm_host_endpoint_step(&host) == WL_OK);
        CHECK(fci_arm_firmware_endpoint_step(&firmware) == WL_OK);
    }
    CHECK(completed == expected);
    return 0;
}
int main(void) {
    uint64_t host_id = 11U, server_id = 22U;
    wl_environment_t environment = {.clock = {clock_now, NULL}, .session = {session, &host_id}};
    fci_arm_firmware_endpoint_config_t config;
    CHECK(fci_arm_host_endpoint_init(&host, environment) == WL_OK);
    environment.session.context = &server_id;
    CHECK(fci_arm_firmware_endpoint_config_defaults(&config, environment) == WL_OK);
    config.on_home = home_handler;
    config.on_clear_faults = clear_handler;
    config.on_set_device_info = rename_handler;
    config.on_get_device_info = info_handler;
    CHECK(fci_arm_firmware_endpoint_init_config(&firmware, &config) == WL_OK);
    CHECK(wl_set_sink(wl_endpoint_link(fci_arm_host_endpoint_handle(&host)), sink,
        fci_arm_firmware_endpoint_handle(&firmware)) == WL_OK);
    CHECK(wl_set_sink(wl_endpoint_link(fci_arm_firmware_endpoint_handle(&firmware)), sink,
        fci_arm_host_endpoint_handle(&host)) == WL_OK);
    set_device_info_request_value_t rename = {.has_custom_name = true};
    rename.custom_name.length = 6U;
    memcpy(rename.custom_name.data, "Willow", 6U);
    CHECK(fci_arm_host_endpoint_set_device_info_async(&host, &rename, 100U,
        set_device_info_done, NULL, NULL) == WL_OK);
    memset(&rename, 0, sizeof(rename)); /* Admission has already copied input. */
    CHECK(drive(1U) == 0 && strcmp(name, "Willow") == 0);
    get_device_info_request_value_t get = {0};
    CHECK(fci_arm_host_endpoint_get_device_info_async(&host, &get, 100U, info_done, NULL, NULL) == WL_OK);
    CHECK(drive(2U) == 0 && last_status == WL_RPC_SUCCESS);
    clear_faults_request_value_t clear = {0};
    CHECK(fci_arm_host_endpoint_clear_faults_async(&host, &clear, 100U, clear_faults_done, NULL, NULL) == WL_OK);
    CHECK(drive(3U) == 0 && last_status == WL_RPC_REJECTED && last_rejection == FAULT_OPERATION_UNSUPPORTED);
    home_request_value_t request = {0};
    for (unsigned i = 0U; i < 12U; ++i) {
        CHECK(fci_arm_host_endpoint_home_async(&host, &request, 100U, home_done, NULL, NULL) == WL_OK);
        CHECK(drive(4U + i) == 0 && last_status == WL_RPC_SUCCESS);
    }
    CHECK(served == 12U); /* More calls than slots, without any release API. */
    CHECK(saved.info.custom_name.length == 6U && memcmp(saved.info.custom_name.data, "Willow", 6U) == 0);
    CHECK(fci_arm_host_endpoint_close(&host) == WL_OK);
    CHECK(fci_arm_firmware_endpoint_close(&firmware) == WL_OK);
    return 0;
}
