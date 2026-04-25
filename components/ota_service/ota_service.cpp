#include "ota_service.hpp"
#include "settings_store.hpp"
#include "app_events.hpp"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_event.h"

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
    http_cfg.url                = cfg.ota_url;
    http_cfg.crt_bundle_attach  = esp_crt_bundle_attach;
    http_cfg.timeout_ms         = 30000;
    http_cfg.keep_alive_enable  = true;

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = &http_cfg;

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful! Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(err));
    }
    return false;
}
