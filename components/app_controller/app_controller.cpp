#include "app_controller.hpp"
#include "app_events.hpp"
#include "clock_engine.hpp"
#include "led_renderer.hpp"
#include "time_service.hpp"
#include "wifi_service.hpp"
#include "provisioning_service.hpp"
#include "ota_service.hpp"
#include "settings_store.hpp"
#include "diagnostics_service.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

// Define the QCLOCKS event base (declared in app_events.hpp).
ESP_EVENT_DEFINE_BASE(QCLOCKS_EVENTS);

static const char *TAG = "app_ctrl";
static DeviceState s_state = DeviceState::BOOT;

// Event group bits for synchronization
#define WIFI_CONNECTED_BIT  BIT0
#define TIME_SYNCED_BIT     BIT2

static EventGroupHandle_t s_event_group = nullptr;

DeviceState app_controller_get_state(void)
{
    return s_state;
}

// -----------------------------------------------------------------------
// Internal: state transition helper
// -----------------------------------------------------------------------
static void transition(DeviceState next)
{
    ESP_LOGI(TAG, "State: %d -> %d", (int)s_state, (int)next);
    s_state = next;
}

// -----------------------------------------------------------------------
// QCLOCKS event handler
// -----------------------------------------------------------------------
static void qclocks_event_handler(void *arg, esp_event_base_t base,
                                  int32_t id, void *data)
{
    (void)arg; (void)base; (void)data;
    auto event = static_cast<QclocksEvent>(id);
    switch (event) {
        case QclocksEvent::WIFI_CONNECTED:
            ESP_LOGI(TAG, "Event: WIFI_CONNECTED");
            if (s_event_group) xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
            break;
        case QclocksEvent::WIFI_DISCONNECTED:
            ESP_LOGI(TAG, "Event: WIFI_DISCONNECTED");
            break;
        case QclocksEvent::TIME_SYNCED:
            ESP_LOGI(TAG, "Event: TIME_SYNCED");
            if (s_event_group) xEventGroupSetBits(s_event_group, TIME_SYNCED_BIT);
            break;
        case QclocksEvent::PROVISIONING_DONE:
            ESP_LOGI(TAG, "Event: PROVISIONING_DONE");
            break;
        case QclocksEvent::OTA_REQUESTED:
            ESP_LOGI(TAG, "Event: OTA_REQUESTED");
            break;
        default:
            break;
    }
}

// -----------------------------------------------------------------------
// Night-mode brightness selection
// -----------------------------------------------------------------------
static uint8_t select_brightness(const AppSettings &cfg, uint8_t h, uint8_t m)
{
    if (!cfg.night_mode_enabled) return cfg.brightness_day;

    uint16_t now_mins   = h * 60u + m;
    uint16_t start_mins = cfg.night_start_hour * 60u + cfg.night_start_minute;
    uint16_t end_mins   = cfg.night_end_hour   * 60u + cfg.night_end_minute;

    bool is_night = false;
    if (start_mins > end_mins) {
        // window crosses midnight
        is_night = (now_mins >= start_mins || now_mins < end_mins);
    } else {
        is_night = (now_mins >= start_mins && now_mins < end_mins);
    }
    return is_night ? cfg.brightness_night : cfg.brightness_day;
}

// -----------------------------------------------------------------------
// Fallback display: blank all LEDs
// -----------------------------------------------------------------------
static void show_fallback(void)
{
    led_renderer_clear();
    led_renderer_show();
}

// -----------------------------------------------------------------------
// Main render loop (RUNNING state)
// Only re-renders when minute changes.
// -----------------------------------------------------------------------
static void render_loop(const AppSettings &cfg)
{
    uint8_t last_minute = 0xFF;

    while (true) {
        struct tm now_tm{};
        if (!time_service_get_local_tm(&now_tm)) {
            show_fallback();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        uint8_t h = static_cast<uint8_t>(now_tm.tm_hour);
        uint8_t m = static_cast<uint8_t>(now_tm.tm_min);

        // Update brightness
        led_renderer_set_brightness(select_brightness(cfg, h, m));

        // Re-render only when minute changes
        if (m != last_minute) {
            last_minute = m;
            auto tokens = render_time_tokens(h, m);
            led_renderer_set_tokens(tokens);
            led_renderer_show();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// -----------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------
void app_controller_start(void)
{
    ESP_LOGI(TAG, "qclocks starting - Phase 2");

    s_event_group = xEventGroupCreate();

    // Register QCLOCKS event handler before any service inits
    esp_event_handler_register(QCLOCKS_EVENTS, ESP_EVENT_ANY_ID,
                                qclocks_event_handler, nullptr);

    // Load settings (NVS already initialised by app_main)
    AppSettings cfg{};
    settings_init();
    settings_load(&cfg);
    ESP_LOGI(TAG, "TZ: %s  brightness day/night: %u/%u",
             cfg.timezone, cfg.brightness_day, cfg.brightness_night);

    // Init display subsystems
    diagnostics_service_init();
    led_renderer_init();
    led_renderer_set_brightness(cfg.brightness_day);

    transition(DeviceState::BOOT);

    // Check Wi-Fi credentials
    provisioning_service_init();
    if (!provisioning_service_has_credentials()) {
        transition(DeviceState::NEEDS_PROVISIONING);
        provisioning_service_start();

        // Show blank display while in provisioning (no time available)
        while (true) {
            show_fallback();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    // Connect to Wi-Fi
    transition(DeviceState::CONNECTING_WIFI);
    wifi_service_init();
    wifi_service_connect();

    // Wait for IP (up to 30 s)
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        transition(DeviceState::ONLINE_SYNCING_TIME);
        time_service_init();

        // Wait for first SNTP sync (up to 30 s)
        bits = xEventGroupWaitBits(s_event_group,
                                   TIME_SYNCED_BIT,
                                   pdFALSE, pdFALSE,
                                   pdMS_TO_TICKS(30000));
        if (!(bits & TIME_SYNCED_BIT)) {
            ESP_LOGW(TAG, "SNTP sync timeout, continuing without valid time");
        }
    } else {
        ESP_LOGW(TAG, "WiFi connection timeout, running without network");
        // Still try to initialise time so TZ is set correctly
        time_service_init();
    }

    transition(DeviceState::RUNNING);
    render_loop(cfg);
}
