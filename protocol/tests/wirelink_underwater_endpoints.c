/* SPDX-License-Identifier: ISC */
#include "fci_device_host_endpoint.h"
#include "fci_device_firmware_endpoint.h"
#include "wirelink/port.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
_Static_assert(sizeof(((underwater_status_value_t *)0)->master_enabled_mask) == 1U,
               "motor masks use exact byte storage");
_Static_assert(sizeof(((underwater_status_value_t *)0)->uptime_ms) == 4U,
               "uptime uses fixed32 storage");
static fci_device_host_endpoint_t host;
static fci_device_firmware_endpoint_t firmware;
static wl_time_ms_t now = 100U;
static unsigned completed;
static wl_rpc_status_t completion_status;
static underwater_result_t business_result;
static underwater_status_value_t current, saved;
static underwater_settings_snapshot_value_t settings_current, settings_saved;
static underwater_settings_result_t settings_result;
static char settings_reason[96];

#define SETTINGS_ARRAYS(X) \
    X(angle_scale) X(angle_offset_rad) X(kp) X(kd) X(gravity_scale) \
    X(friction_coulomb_positive_nm) X(friction_coulomb_negative_nm) \
    X(friction_viscous_nm_per_rad_s) X(friction_deadband_rad_s) \
    X(friction_transition_rad_s) X(friction_filter_time_constant_s) \
    X(force_gain) X(force_low_gain) X(force_velocity_filter_s) X(force_load_filter_s)

static void fill_settings(underwater_settings_value_t *value) {
    float number = -16.0f;
#define FILL(field) \
    value->has_##field = true; \
    for (size_t i = 0U; i < sizeof(value->field) / sizeof(value->field[0]); ++i) \
        value->field[i] = number += 0.25f;
    SETTINGS_ARRAYS(FILL)
#undef FILL
    value->has_force_low_load_nm = value->has_force_high_load_nm = true;
    value->has_buzzer_alarm_enabled = true;
    value->buzzer_alarm_enabled = true;
    value->force_low_load_nm = 2.0f; value->force_high_load_nm = 5.0f;
}

static int equal_settings(const underwater_settings_value_t *a, const underwater_settings_value_t *b) {
#define EQUAL(field) CHECK(a->has_##field == b->has_##field && memcmp(a->field, b->field, sizeof(a->field)) == 0);
    SETTINGS_ARRAYS(EQUAL)
#undef EQUAL
    CHECK(a->force_low_load_nm == b->force_low_load_nm);
    CHECK(a->force_high_load_nm == b->force_high_load_nm);
    CHECK(a->has_buzzer_alarm_enabled == b->has_buzzer_alarm_enabled);
    CHECK(a->buzzer_alarm_enabled == b->buzzer_alarm_enabled);
    return 0;
}

#define SETTINGS_RESPONSE(out, text) do { \
    (out)->has_result = (out)->has_snapshot = (out)->has_reason = true; \
    (out)->result = UNDERWATER_SETTINGS_OK; (out)->snapshot = settings_current; \
    (out)->reason.length = strlen(text); memcpy((out)->reason.data, text, (out)->reason.length); \
} while (0)
static int32_t get_settings(void *context, const get_underwater_settings_request_value_t *request,
                            get_underwater_settings_response_value_t *response) {
    (void)context; (void)request;
    SETTINGS_RESPONSE(response, "active settings");
    return 0;
}
static int32_t set_settings(void *context, const set_underwater_settings_request_value_t *request,
                            set_underwater_settings_response_value_t *response) {
    (void)context;
    if (!current.upgrading) {
        settings_current.settings = request->settings;
        settings_current.revision++;
        settings_current.source = UNDERWATER_SETTINGS_SOURCE_STORED;
    }
    SETTINGS_RESPONSE(response, "saved and applied");
    if (current.upgrading) {
        response->result = UNDERWATER_SETTINGS_BUSY;
        const char *text = "upgrade active; settings unchanged";
        response->reason.length = strlen(text);
        memcpy(response->reason.data, text, response->reason.length);
    }
    return 0;
}
static int32_t get_defaults(void *context, const get_underwater_default_settings_request_value_t *request,
                            get_underwater_default_settings_response_value_t *response) {
    (void)context; (void)request;
    SETTINGS_RESPONSE(response, "firmware defaults");
    fill_settings(&response->snapshot.settings);
    response->snapshot.revision = 0;
    response->snapshot.source = UNDERWATER_SETTINGS_SOURCE_DEFAULTS;
    return 0;
}
#undef SETTINGS_RESPONSE
#define SETTINGS_COMPLETION(service) \
static void service##_done(void *context, const wl_rpc_completion_t *result, \
                           const service##_response_value_t *response) { \
    (void)context; ++completed; completion_status = result->status; \
    if (response != NULL) { \
        settings_result = response->result; settings_saved = response->snapshot; \
        memcpy(settings_reason, response->reason.data, response->reason.length); \
        settings_reason[response->reason.length] = '\0'; \
    } \
}
SETTINGS_COMPLETION(get_underwater_settings)
SETTINGS_COMPLETION(set_underwater_settings)
SETTINGS_COMPLETION(get_underwater_default_settings)
#undef SETTINGS_COMPLETION

static wl_time_ms_t clock_now(void *context) { (void)context; return now; }
static wl_err_t session(void *context, uint64_t *out) {
    *out = *(const uint64_t *)context;
    return WL_OK;
}
static wl_sink_result_t sink(void *peer, wl_io_token_t token,
                             const uint8_t *data, size_t length) {
    size_t accepted = 0U;
    (void)token;
    return wl_feed_bytes(wl_endpoint_link(peer), data, length, &accepted) == WL_OK
        && accepted == length ? WL_SINK_SENT : WL_SINK_FAILED;
}
static void reason(const char *text) {
    current.reason.length = strlen(text);
    memcpy(current.reason.data, text, current.reason.length);
}
static int32_t get_status(void *context,
                         const get_underwater_status_request_value_t *request,
                         get_underwater_status_response_value_t *response) {
    (void)context; (void)request;
    response->has_result = response->has_info = true;
    response->result = UNDERWATER_RESULT_OK;
    response->info = current;
    return 0;
}
static int32_t set_mode(void *context,
                       const set_underwater_mode_request_value_t *request,
                       set_underwater_mode_response_value_t *response) {
    (void)context;
    response->has_result = response->has_info = true;
    if (current.upgrading) {
        response->result = UNDERWATER_RESULT_BUSY;
        reason("upgrade active; remain in Damp");
    } else if (request->mode != UNDERWATER_MODE_DAMP &&
               request->mode != UNDERWATER_MODE_GRAVITY &&
               request->mode != UNDERWATER_MODE_TELEOP) {
        response->result = UNDERWATER_RESULT_INVALID_MODE;
        reason("requested mode is not remotely selectable");
    } else {
        response->result = UNDERWATER_RESULT_OK;
        current.mode = request->mode;
        current.enabling_arms = 1U;
        reason("master enabling; slave feedback absent");
    }
    response->info = current;
    return 0; /* Preserve business rejection details in the response DTO. */
}
#define COMPLETION(service) \
static void service##_done(void *context, const wl_rpc_completion_t *result, \
                           const service##_response_value_t *response) { \
    (void)context; ++completed; completion_status = result->status; \
    if (response != NULL) { business_result = response->result; saved = response->info; } \
}
COMPLETION(get_underwater_status)
COMPLETION(set_underwater_mode)
#undef COMPLETION
static int drive(unsigned expected) {
    for (unsigned i = 0U; i < 24U && completed < expected; ++i, ++now) {
        CHECK(fci_device_host_endpoint_step(&host) == WL_OK);
        CHECK(fci_device_firmware_endpoint_step(&firmware) == WL_OK);
    }
    CHECK(completed == expected);
    CHECK(completion_status == WL_RPC_SUCCESS);
    return 0;
}
int main(void) {
    uint64_t host_id = 11U, firmware_id = 22U;
    wl_environment_t environment = {.clock = {clock_now, NULL}, .session = {session, &host_id}};
    fci_device_firmware_endpoint_config_t config;
    CHECK(fci_device_host_endpoint_init(&host, environment) == WL_OK);
    environment.session.context = &firmware_id;
    CHECK(fci_device_firmware_endpoint_config_defaults(&config, environment) == WL_OK);
    config.on_get_underwater_status = get_status;
    config.on_set_underwater_mode = set_mode;
    config.on_get_underwater_settings = get_settings;
    config.on_set_underwater_settings = set_settings;
    config.on_get_underwater_default_settings = get_defaults;
    CHECK(fci_device_firmware_endpoint_init_config(&firmware, &config) == WL_OK);
    CHECK(wl_set_sink(wl_endpoint_link(fci_device_host_endpoint_handle(&host)), sink,
        fci_device_firmware_endpoint_handle(&firmware)) == WL_OK);
    CHECK(wl_set_sink(wl_endpoint_link(fci_device_firmware_endpoint_handle(&firmware)), sink,
        fci_device_host_endpoint_handle(&host)) == WL_OK);
    current.has_mode = current.has_uptime_ms = current.has_enabling_arms = true;
    current.has_gravity_arms = current.has_master_available_mask = true;
    current.has_slave_available_mask = current.has_master_enabled_mask = true;
    current.has_slave_enabled_mask = current.has_master_disable_pending_mask = true;
    current.has_slave_disable_pending_mask = current.has_upgrading = current.has_reason = true;
    current.mode = UNDERWATER_MODE_DAMP;
    current.uptime_ms = UINT32_MAX;
    current.master_available_mask = 0x1fU;
    current.slave_disable_pending_mask = 0x1fU;
    current.has_master_disabled_mask = current.has_slave_disabled_mask = true;
    current.master_disabled_mask = 0x15U;
    reason("Damp");
    get_underwater_status_request_value_t get = {0};
    CHECK(fci_device_host_endpoint_get_underwater_status_async(&host, &get, 100U,
        get_underwater_status_done, NULL, NULL) == WL_OK);
    CHECK(drive(1U) == 0 && business_result == UNDERWATER_RESULT_OK);
    CHECK(saved.mode == UNDERWATER_MODE_DAMP && saved.uptime_ms == UINT32_MAX);
    CHECK(saved.master_available_mask == 0x1fU && saved.master_enabled_mask == 0U);
    CHECK(saved.slave_disable_pending_mask == 0x1fU);
    CHECK(saved.has_master_disabled_mask && saved.master_disabled_mask == 0x15U);
    CHECK(saved.has_slave_disabled_mask && saved.slave_disabled_mask == 0U);
    set_underwater_mode_request_value_t set = {.has_mode = true, .mode = UNDERWATER_MODE_GRAVITY};
    CHECK(fci_device_host_endpoint_set_underwater_mode_async(&host, &set, 100U,
        set_underwater_mode_done, NULL, NULL) == WL_OK);
    set.mode = UNDERWATER_MODE_CONTROL; /* The admitted call owns its request. */
    CHECK(drive(2U) == 0 && business_result == UNDERWATER_RESULT_OK);
    CHECK(saved.mode == UNDERWATER_MODE_GRAVITY && saved.enabling_arms == 1U);
    CHECK(saved.master_enabled_mask == 0U); /* Accepted, not yet enabled. */
    CHECK(fci_device_host_endpoint_set_underwater_mode_async(&host, &set, 100U,
        set_underwater_mode_done, NULL, NULL) == WL_OK);
    CHECK(drive(3U) == 0 && business_result == UNDERWATER_RESULT_INVALID_MODE);
    CHECK(saved.mode == UNDERWATER_MODE_GRAVITY && saved.reason.length != 0U);
    current.upgrading = true;
    current.mode = UNDERWATER_MODE_DAMP;
    current.enabling_arms = 0U;
    set.mode = UNDERWATER_MODE_TELEOP;
    CHECK(fci_device_host_endpoint_set_underwater_mode_async(&host, &set, 100U,
        set_underwater_mode_done, NULL, NULL) == WL_OK);
    CHECK(drive(4U) == 0 && business_result == UNDERWATER_RESULT_BUSY);
    CHECK(saved.mode == UNDERWATER_MODE_DAMP && saved.upgrading);
    CHECK(saved.reason.length == strlen("upgrade active; remain in Damp"));
    CHECK(memcmp(saved.reason.data, "upgrade active; remain in Damp", saved.reason.length) == 0);
    /* Owned response strings survive later calls using the shared scratch. */
    reason("different later state");
    CHECK(memcmp(saved.reason.data, "upgrade active; remain in Damp", saved.reason.length) == 0);
    settings_current.has_settings = settings_current.has_revision = true;
    settings_current.has_source = settings_current.has_storage_error = true;
    fill_settings(&settings_current.settings);
    get_underwater_settings_request_value_t settings_get = {0};
    CHECK(fci_device_host_endpoint_get_underwater_settings_async(&host, &settings_get, 100U,
        get_underwater_settings_done, NULL, NULL) == WL_OK);
    CHECK(drive(5U) == 0 && settings_result == UNDERWATER_SETTINGS_OK);
    CHECK(equal_settings(&settings_current.settings, &settings_saved.settings) == 0);
    set_underwater_settings_request_value_t settings_set = {.has_settings = true};
    settings_set.settings = settings_current.settings;
    settings_set.settings.angle_scale[2] = -2.0f;
    settings_set.settings.buzzer_alarm_enabled = false;
    current.upgrading = false; current.mode = UNDERWATER_MODE_TELEOP;
    CHECK(fci_device_host_endpoint_set_underwater_settings_async(&host, &settings_set, 100U,
        set_underwater_settings_done, NULL, NULL) == WL_OK);
    settings_set.settings.angle_scale[2] = 9.0f; /* Async admission owns the entire settings object. */
    settings_set.settings.buzzer_alarm_enabled = true;
    CHECK(drive(6U) == 0 && settings_result == UNDERWATER_SETTINGS_OK);
    CHECK(settings_saved.settings.angle_scale[2] == -2.0f && settings_saved.revision == 1U);
    CHECK(settings_saved.source == UNDERWATER_SETTINGS_SOURCE_STORED);
    CHECK(settings_saved.settings.has_buzzer_alarm_enabled && !settings_saved.settings.buzzer_alarm_enabled);
    current.upgrading = true;
    CHECK(fci_device_host_endpoint_set_underwater_settings_async(&host, &settings_set, 100U,
        set_underwater_settings_done, NULL, NULL) == WL_OK);
    CHECK(drive(7U) == 0 && settings_result == UNDERWATER_SETTINGS_BUSY);
    CHECK(settings_saved.settings.angle_scale[2] == -2.0f && settings_saved.revision == 1U);
    CHECK(strcmp(settings_reason, "upgrade active; settings unchanged") == 0);
    get_underwater_default_settings_request_value_t settings_defaults = {0};
    CHECK(fci_device_host_endpoint_get_underwater_default_settings_async(&host, &settings_defaults, 100U,
        get_underwater_default_settings_done, NULL, NULL) == WL_OK);
    CHECK(drive(8U) == 0 && settings_result == UNDERWATER_SETTINGS_OK);
    CHECK(settings_saved.source == UNDERWATER_SETTINGS_SOURCE_DEFAULTS && settings_saved.revision == 0U);
    CHECK(settings_current.settings.angle_scale[2] == -2.0f && settings_current.revision == 1U);
    set_underwater_settings_response_value_t full_response = {
        .has_result = true, .result = UNDERWATER_SETTINGS_STORAGE_FAILED,
        .has_snapshot = true, .snapshot = settings_current, .has_reason = true
    };
    full_response.reason.length = 95U; memset(full_response.reason.data, 'x', 95U);
    full_response.snapshot.storage_error = INT32_MIN;
    full_response.snapshot.revision = UINT32_MAX;
    uint8_t full_encoded[2048]; size_t full_length = 0U;
    CHECK(set_underwater_settings_response_value_encode(&full_response, full_encoded,
        sizeof(full_encoded), &full_length) == WL_CODEC_OK);
    CHECK(full_length + 20U <= 2048U); /* Includes managed v2 header. */
    full_response.snapshot.settings.has_force_load_filter_s = false;
    CHECK(set_underwater_settings_response_value_encode(&full_response, full_encoded,
        sizeof(full_encoded), &full_length) != WL_CODEC_OK);
    uint8_t encoded[256];
    size_t length = 0U;
    set.has_mode = false;
    CHECK(set_underwater_mode_request_value_encode(&set, encoded, sizeof(encoded), &length) != WL_CODEC_OK);
    current.has_reason = false;
    CHECK(underwater_status_value_encode(&current, encoded, sizeof(encoded), &length) != WL_CODEC_OK);
    current.has_reason = true;
    current.reason.length = 96U;
    CHECK(underwater_status_value_encode(&current, encoded, sizeof(encoded), &length) != WL_CODEC_OK);
    CHECK(fci_device_host_endpoint_close(&host) == WL_OK);
    CHECK(fci_device_firmware_endpoint_close(&firmware) == WL_OK);
    return 0;
}
