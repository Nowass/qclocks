#include "settings_store.hpp"
#include <cstring>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "settings";
static const char *NVS_NAMESPACE = "qclocks";

// Default values applied on first boot (spec section 15.3).
static void apply_defaults(AppSettings *s)
{
    s->brightness_day     = 96;
    s->brightness_night   = 24;
    s->night_mode_enabled = true;
    s->night_start_hour   = 22;
    s->night_start_minute = 0;
    s->night_end_hour     = 6;
    s->night_end_minute   = 0;
    strncpy(s->timezone, "CET-1CEST,M3.5.0/2,M10.5.0/3", sizeof(s->timezone) - 1);
    s->timezone[sizeof(s->timezone) - 1] = '\0';
    s->ota_url[0] = '\0';
    strncpy(s->hostname, "qclocks", sizeof(s->hostname) - 1);
    s->hostname[sizeof(s->hostname) - 1] = '\0';
}

bool settings_init(void)
{
    ESP_LOGI(TAG, "settings init");
    return true;
}

bool settings_load(AppSettings *out)
{
    if (!out) return false;

    apply_defaults(out);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "first boot, using defaults");
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }

    uint8_t u8_val;

    if (nvs_get_u8(handle, "bright_day",   &u8_val) == ESP_OK)
        out->brightness_day = u8_val;

    if (nvs_get_u8(handle, "bright_night",  &u8_val) == ESP_OK)
        out->brightness_night = u8_val;

    if (nvs_get_u8(handle, "night_en",      &u8_val) == ESP_OK)
        out->night_mode_enabled = (u8_val != 0);

    if (nvs_get_u8(handle, "night_sh",      &u8_val) == ESP_OK)
        out->night_start_hour = u8_val;

    if (nvs_get_u8(handle, "night_sm",      &u8_val) == ESP_OK)
        out->night_start_minute = u8_val;

    if (nvs_get_u8(handle, "night_eh",      &u8_val) == ESP_OK)
        out->night_end_hour = u8_val;

    if (nvs_get_u8(handle, "night_em",      &u8_val) == ESP_OK)
        out->night_end_minute = u8_val;

    size_t sz = sizeof(out->timezone);
    nvs_get_str(handle, "timezone", out->timezone, &sz);

    sz = sizeof(out->ota_url);
    nvs_get_str(handle, "ota_url", out->ota_url, &sz);

    sz = sizeof(out->hostname);
    nvs_get_str(handle, "hostname", out->hostname, &sz);

    nvs_close(handle);

    ESP_LOGI(TAG, "loaded: bright_day=%u bright_night=%u night_en=%d "
             "night=%02u:%02u-%02u:%02u tz=%s hostname=%s",
             out->brightness_day, out->brightness_night,
             (int)out->night_mode_enabled,
             out->night_start_hour, out->night_start_minute,
             out->night_end_hour,   out->night_end_minute,
             out->timezone, out->hostname);

    return true;
}

bool settings_save(const AppSettings *in)
{
    if (!in) return false;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open (rw) failed: %s", esp_err_to_name(err));
        return false;
    }

    bool ok = true;

    ok &= (nvs_set_u8(handle, "bright_day",   in->brightness_day)              == ESP_OK);
    ok &= (nvs_set_u8(handle, "bright_night",  in->brightness_night)            == ESP_OK);
    ok &= (nvs_set_u8(handle, "night_en",      (uint8_t)in->night_mode_enabled) == ESP_OK);
    ok &= (nvs_set_u8(handle, "night_sh",      in->night_start_hour)            == ESP_OK);
    ok &= (nvs_set_u8(handle, "night_sm",      in->night_start_minute)          == ESP_OK);
    ok &= (nvs_set_u8(handle, "night_eh",      in->night_end_hour)              == ESP_OK);
    ok &= (nvs_set_u8(handle, "night_em",      in->night_end_minute)            == ESP_OK);
    ok &= (nvs_set_str(handle, "timezone",     in->timezone)                    == ESP_OK);
    ok &= (nvs_set_str(handle, "ota_url",      in->ota_url)                     == ESP_OK);
    ok &= (nvs_set_str(handle, "hostname",     in->hostname)                    == ESP_OK);

    if (!ok) {
        ESP_LOGE(TAG, "one or more nvs_set calls failed");
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "settings saved");
    return true;
}

bool settings_reset(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open (reset) failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_erase_all(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_erase_all failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit (reset) failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "settings reset to defaults");
    return true;
}
