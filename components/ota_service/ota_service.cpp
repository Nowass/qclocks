#include "ota_service.hpp"
#include "settings_store.hpp"
#include "app_events.hpp"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_app_desc.h"

static const char *TAG = "ota_svc";

bool ota_service_init(void)
{
    ESP_LOGI(TAG, "OTA service init");
    return true;
}

bool ota_service_check_for_update(void)
{
    AppSettings cfg = {};
    if (!settings_load(&cfg)) {
        ESP_LOGW(TAG, "Failed to load settings");
        return false;
    }
    if (cfg.ota_url[0] == '\0') {
        ESP_LOGW(TAG, "OTA URL not configured in NVS. Set key 'ota_url' in namespace 'qclocks'");
        return false;
    }
    ESP_LOGI(TAG, "OTA URL: %s", cfg.ota_url);
    esp_event_post(QCLOCKS_EVENTS, (int32_t)QclocksEvent::OTA_REQUESTED, nullptr, 0, 0);
    return true;
}

bool ota_service_start_update(void)
{
    AppSettings cfg = {};
    if (!settings_load(&cfg)) {
        ESP_LOGE(TAG, "Failed to load settings");
        return false;
    }
    if (cfg.ota_url[0] == '\0') {
        ESP_LOGE(TAG, "OTA URL is empty, cannot start update");
        return false;
    }
    ESP_LOGI(TAG, "Starting OTA update from: %s", cfg.ota_url);

    esp_http_client_config_t http_cfg = {};
    http_cfg.url                   = cfg.ota_url;
    http_cfg.crt_bundle_attach     = esp_crt_bundle_attach;
    http_cfg.timeout_ms            = 30000;
    http_cfg.keep_alive_enable     = true;
    http_cfg.max_redirection_count = 5;    // GitHub releases redirects to CDN
    http_cfg.buffer_size           = 4096; // GitHub CDN response headers can exceed 512 B default
    http_cfg.buffer_size_tx        = 2048;

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config            = &http_cfg;

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful! Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(err));
    }
    return false;
}

bool ota_service_check_and_update(void)
{
    AppSettings cfg = {};
    if (!settings_load(&cfg)) {
        ESP_LOGE(TAG, "Failed to load settings");
        return false;
    }
    if (cfg.ota_url[0] == '\0') {
        ESP_LOGW(TAG, "OTA URL not set, skipping auto-check");
        return false;
    }

    ESP_LOGI(TAG, "Auto OTA check: %s", cfg.ota_url);

    esp_http_client_config_t http_cfg = {};
    http_cfg.url                   = cfg.ota_url;
    http_cfg.crt_bundle_attach     = esp_crt_bundle_attach;
    http_cfg.timeout_ms            = 30000;
    http_cfg.keep_alive_enable     = true;
    http_cfg.max_redirection_count = 5;
    http_cfg.buffer_size           = 4096;
    http_cfg.buffer_size_tx        = 2048;

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config            = &http_cfg;

    esp_https_ota_handle_t handle = nullptr;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return false;
    }

    // Read new firmware app descriptor (downloads only first ~1 KB)
    esp_app_desc_t new_desc = {};
    err = esp_https_ota_get_img_desc(handle, &new_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read new firmware descriptor: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        return false;
    }

    // Compare with running version
    const esp_app_desc_t *running = esp_app_get_description();
    if (strcmp(new_desc.version, running->version) == 0) {
        printf("OTA: firmware is up to date (%s)\r\n", running->version);
        esp_https_ota_abort(handle);
        return false;
    }

    printf("OTA: new version %s (running %s) – updating...\r\n",
           new_desc.version, running->version);

    // Download and flash
    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;
    }

    if (err == ESP_OK) {
        err = esp_https_ota_finish(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA successful, rebooting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
    }
    return false;
}
