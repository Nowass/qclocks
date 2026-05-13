#include "wifi_service.hpp"
#include "app_events.hpp"
#include "app_types.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs.h"
#include <cstring>

static const char *TAG = "wifi_service";

static volatile bool s_connected   = false;
static volatile int  s_retry_count = 0;
static constexpr int MAX_RETRY     = 5;

enum class WifiPhase { PRIMARY, FALLBACK, FAILED };
static WifiPhase s_wifi_phase = WifiPhase::PRIMARY;

// Switch Wi-Fi config to the hardcoded fallback network and reconnect.
static void switch_to_fallback(void)
{
    wifi_config_t cfg = {};
    strlcpy(reinterpret_cast<char *>(cfg.sta.ssid),
            FALLBACK_WIFI_SSID, sizeof(cfg.sta.ssid));
    strlcpy(reinterpret_cast<char *>(cfg.sta.password),
            FALLBACK_WIFI_PASS, sizeof(cfg.sta.password));
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    s_retry_count = 0;
    s_wifi_phase  = WifiPhase::FALLBACK;
    ESP_LOGI("wifi_service", "Primary failed – trying fallback SSID: %s",
             FALLBACK_WIFI_SSID);
    esp_wifi_connect();
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi connected to AP");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        esp_event_post(QCLOCKS_EVENTS,
                       (int32_t)QclocksEvent::WIFI_DISCONNECTED,
                       nullptr, 0, 0);
        if (s_retry_count < MAX_RETRY) {
            ESP_LOGW(TAG, "Disconnected, retry %d/%d", s_retry_count + 1, MAX_RETRY);
            s_retry_count = s_retry_count + 1;
            esp_wifi_connect();
        } else if (s_wifi_phase == WifiPhase::PRIMARY &&
                   FALLBACK_WIFI_SSID[0] != '\0') {
            // Primary exhausted – try fallback network.
            switch_to_fallback();
        } else {
            ESP_LOGE(TAG, "Max retries reached on all configured networks");
            s_wifi_phase = WifiPhase::FAILED;
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
    s_wifi_phase  = WifiPhase::PRIMARY;

    if (esp_wifi_start() != ESP_OK) {
        return false;
    }

    // Limit TX power to 8 dBm to prevent brownout on boards with marginal
    // power supply. Unit: 0.25 dBm steps (32 = 8 dBm, 64 = 16 dBm).
    esp_wifi_set_max_tx_power(32);

    // Modem sleep: radio is off when not actively TX/RX.
    // Reduces average current and WiFi interrupt load on the CPU
    // (which also prevents USB-CDC write timeouts on the host side).
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

    ESP_LOGI(TAG, "WiFi TX capped at 8 dBm, modem sleep enabled");

    ESP_LOGI(TAG, "Connecting to SSID: %s",
             reinterpret_cast<const char *>(wifi_cfg.sta.ssid));
    return true;
}

bool wifi_service_is_connected(void)
{
    return s_connected;
}
