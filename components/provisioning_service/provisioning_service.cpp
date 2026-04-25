#include "provisioning_service.hpp"
#include "esp_log.h"
#include "nvs.h"
#include <cstring>

static const char *TAG = "prov";

bool provisioning_service_init(void)
{
    ESP_LOGI(TAG, "provisioning service init");
    return true;
}

bool provisioning_service_has_credentials(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi_creds", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "has_credentials: no");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }

    char ssid[33] = {};
    size_t len = sizeof(ssid);
    err = nvs_get_str(handle, "ssid", ssid, &len);
    nvs_close(handle);

    bool found = (err == ESP_OK && ssid[0] != '\0');
    ESP_LOGD(TAG, "has_credentials: %s", found ? "yes" : "no");
    return found;
}

bool provisioning_service_start(void)
{
    ESP_LOGW(TAG, "Provisioning mode: no Wi-Fi credentials stored in NVS");
    ESP_LOGW(TAG, "Connect to SoftAP 'qclocks-setup' and configure via HTTP (Phase 3 - not yet implemented)");
    ESP_LOGW(TAG, "To set credentials for testing, use UART or flash via NVS tool");
    return false;
}

bool provisioning_service_reset(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi_creds", NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
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
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Wi-Fi credentials erased");
    return true;
}
