#include "wifi_service.hpp"
#include "app_events.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs.h"

static const char *TAG = "wifi_service";

static volatile bool s_connected   = false;
static volatile int  s_retry_count = 0;
static constexpr int MAX_RETRY     = 5;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi connected to AP");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        ESP_LOGW(TAG, "Disconnected, retry %d/%d", s_retry_count + 1, MAX_RETRY);
        esp_event_post(QCLOCKS_EVENTS,
                       (int32_t)QclocksEvent::WIFI_DISCONNECTED,
                       nullptr, 0, 0);
        if (s_retry_count < MAX_RETRY) {
            s_retry_count = s_retry_count + 1;
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Max retries reached");
        }
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_connected   = true;
        s_retry_count = 0;
        esp_event_post(QCLOCKS_EVENTS,
                       (int32_t)QclocksEvent::WIFI_CONNECTED,
                       nullptr, 0, 0);
    }
}

bool wifi_service_init(void)
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        return false;
    }

    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                            &wifi_event_handler, nullptr,
                                            nullptr) != ESP_OK) {
        return false;
    }
    if (esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                            &ip_event_handler, nullptr,
                                            nullptr) != ESP_OK) {
        return false;
    }

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        return false;
    }

    ESP_LOGI(TAG, "Wi-Fi service initialized");
    return true;
}

bool wifi_service_connect(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi_creds", NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No Wi-Fi credentials in NVS");
        return false;
    }

    wifi_config_t wifi_cfg = {};

    size_t ssid_len = sizeof(wifi_cfg.sta.ssid);
    err = nvs_get_str(nvs, "ssid",
                      reinterpret_cast<char *>(wifi_cfg.sta.ssid),
                      &ssid_len);
    if (err != ESP_OK || wifi_cfg.sta.ssid[0] == '\0') {
        nvs_close(nvs);
        return false;
    }

    size_t pass_len = sizeof(wifi_cfg.sta.password);
    nvs_get_str(nvs, "password",
                reinterpret_cast<char *>(wifi_cfg.sta.password),
                &pass_len);

    nvs_close(nvs);

    if (esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg) != ESP_OK) {
        return false;
    }

    s_retry_count = 0;

    if (esp_wifi_start() != ESP_OK) {
        return false;
    }

    // Reduce TX power to ~80% of max (16 dBm) to prevent brownout resets
    // on boards with marginal power supply. Unit: 0.25 dBm steps (64 = 16 dBm).
    esp_wifi_set_max_tx_power(64);
    ESP_LOGI(TAG, "WiFi TX power limited to 16 dBm (brownout workaround)");

    ESP_LOGI(TAG, "Connecting to SSID: %s",
             reinterpret_cast<const char *>(wifi_cfg.sta.ssid));
    return true;
}

bool wifi_service_is_connected(void)
{
    return s_connected;
}
