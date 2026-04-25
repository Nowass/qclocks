#include "app_controller.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    // Initialize NVS flash (required by WiFi driver and settings_store)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase, retrying");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return;
    }

    // Initialize TCP/IP stack (required by WiFi and SNTP)
    esp_netif_init();

    // Create default event loop (required by WiFi and QCLOCKS events)
    esp_event_loop_create_default();

    ESP_LOGI(TAG, "Platform init complete, starting app_controller");
    app_controller_start();
}
