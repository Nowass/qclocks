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

#include "word_tokens.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"

// Define the QCLOCKS event base (declared in app_events.hpp).
ESP_EVENT_DEFINE_BASE(QCLOCKS_EVENTS);

static const char *TAG = "app_ctrl";
static DeviceState s_state = DeviceState::BOOT;

// Event group bits for synchronization
#define WIFI_CONNECTED_BIT BIT0
#define TIME_SYNCED_BIT BIT2

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
    (void)arg;
    (void)base;
    (void)data;
    auto event = static_cast<QclocksEvent>(id);
    switch (event)
    {
    case QclocksEvent::WIFI_CONNECTED:
        ESP_LOGI(TAG, "Event: WIFI_CONNECTED");
        if (s_event_group)
            xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
        break;
    case QclocksEvent::WIFI_DISCONNECTED:
        ESP_LOGI(TAG, "Event: WIFI_DISCONNECTED");
        break;
    case QclocksEvent::TIME_SYNCED:
        ESP_LOGI(TAG, "Event: TIME_SYNCED");
        if (s_event_group)
            xEventGroupSetBits(s_event_group, TIME_SYNCED_BIT);
        break;
    case QclocksEvent::PROVISIONING_DONE:
        ESP_LOGI(TAG, "Event: PROVISIONING_DONE");
        break;
    case QclocksEvent::OTA_REQUESTED:
        ESP_LOGI(TAG, "Event: OTA_REQUESTED – spawning OTA task");
        transition(DeviceState::OTA_IN_PROGRESS);
        xTaskCreate([](void *)
                    {
                ESP_LOGI("app_ctrl", "OTA task started");
                if (!ota_service_start_update()) {
                    ESP_LOGE("app_ctrl", "OTA failed, resuming normal operation");
                    // transition back – ota_service_start_update() reboots on success
                    s_state = DeviceState::RUNNING;
                }
                vTaskDelete(nullptr); }, "ota_task", 8192, nullptr, 5, nullptr);
        break;
    default:
        break;
    }
}

// -----------------------------------------------------------------------
// Token → Czech display word
// -----------------------------------------------------------------------
static const char *token_to_czech(TokenId tok)
{
    switch (tok)
    {
    case TokenId::PREFIX_JE:
        return "JE";
    case TokenId::PREFIX_JSOU:
        return "JSOU";
    case TokenId::PREFIX_BUDE:
        return "BUDE";
    case TokenId::HOUR_1_JEDNA:
        return "JEDNA";
    case TokenId::HOUR_2_DVE:
        return "DVĚ";
    case TokenId::HOUR_3_TRI:
        return "TŘI";
    case TokenId::HOUR_4_CTYRI:
        return "ČTYŘI";
    case TokenId::HOUR_5_PET:
        return "PĚT";
    case TokenId::HOUR_6_SEST:
        return "ŠEST";
    case TokenId::HOUR_7_SEDM:
        return "SEDM";
    case TokenId::HOUR_8_OSM:
        return "OSM";
    case TokenId::HOUR_9_DEVET:
        return "DEVĚT";
    case TokenId::HOUR_10_DESET:
        return "DESET";
    case TokenId::HOUR_11_JEDENACT:
        return "JEDENÁCT";
    case TokenId::HOUR_12_DVANACT:
        return "DVANÁCT";
    case TokenId::MIN_0_NULA:
        return "NULA";
    case TokenId::MIN_10_DESET:
        return "DESET";
    case TokenId::MIN_15_PATNACT:
        return "PATNÁCT";
    case TokenId::MIN_20_DVACET:
        return "DVACET";
    case TokenId::MIN_30_TRICET:
        return "TŘICET";
    case TokenId::MIN_40_CTYRICET:
        return "ČTYŘICET";
    case TokenId::MIN_50_PADESAT:
        return "PADESÁT";
    case TokenId::UNIT_1_JEDNA:
        return "JEDNA";
    case TokenId::UNIT_2_DVE:
        return "DVĚ";
    case TokenId::UNIT_3_TRI:
        return "TŘI";
    case TokenId::UNIT_4_CTYRI:
        return "ČTYŘI";
    case TokenId::UNIT_5_PET:
        return "PĚT";
    case TokenId::UNIT_6_SEST:
        return "ŠEST";
    case TokenId::UNIT_7_SEDM:
        return "SEDM";
    case TokenId::UNIT_8_OSM:
        return "OSM";
    case TokenId::UNIT_9_DEVET:
        return "DEVĚT";
    default:
        return "?";
    }
}

// -----------------------------------------------------------------------
// Night-mode brightness selection
// -----------------------------------------------------------------------
static uint8_t select_brightness(const AppSettings &cfg, uint8_t h, uint8_t m)
{
    if (!cfg.night_mode_enabled)
        return cfg.brightness_day;

    uint16_t now_mins = h * 60u + m;
    uint16_t start_mins = cfg.night_start_hour * 60u + cfg.night_start_minute;
    uint16_t end_mins = cfg.night_end_hour * 60u + cfg.night_end_minute;

    bool is_night = false;
    if (start_mins > end_mins)
    {
        // window crosses midnight
        is_night = (now_mins >= start_mins || now_mins < end_mins);
    }
    else
    {
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

    while (true)
    {
        struct tm now_tm{};
        if (!time_service_get_local_tm(&now_tm))
        {
            show_fallback();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        uint8_t h = static_cast<uint8_t>(now_tm.tm_hour);
        uint8_t m = static_cast<uint8_t>(now_tm.tm_min);

        // Update brightness
        led_renderer_set_brightness(select_brightness(cfg, h, m));

        // Re-render only when minute changes
        if (m != last_minute)
        {
            last_minute = m;
            auto tokens = render_time_tokens(h, m);
            led_renderer_set_tokens(tokens);
            led_renderer_show();
            // Print Czech words being displayed
            printf("Display [%02d:%02d]:", h, m);
            for (TokenId tok : tokens)
            {
                printf(" %s", token_to_czech(tok));
            }
            printf("\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// -----------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------
void app_controller_start(void)
{
    ESP_LOGI(TAG, "qclocks starting - Phase 3");

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
    if (!provisioning_service_has_credentials())
    {
        transition(DeviceState::NEEDS_PROVISIONING);
        provisioning_service_start();

        // Show blank display while in provisioning (no time available)
        while (true)
        {
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

    if (bits & WIFI_CONNECTED_BIT)
    {
        printf("WiFi: connected\r\n");
        transition(DeviceState::ONLINE_SYNCING_TIME);
        time_service_init();

        // Wait for first SNTP sync (up to 30 s)
        bits = xEventGroupWaitBits(s_event_group,
                                   TIME_SYNCED_BIT,
                                   pdFALSE, pdFALSE,
                                   pdMS_TO_TICKS(30000));
        if (bits & TIME_SYNCED_BIT)
        {
            struct tm now_tm{};
            time_service_get_local_tm(&now_tm);
            printf("SNTP: synchronized – local time %02d:%02d:%02d\r\n",
                   now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
        }
        else
        {
            ESP_LOGW(TAG, "SNTP sync timeout, continuing without valid time");
            printf("SNTP: sync timeout, running without valid time\r\n");
        }

        // Auto OTA check: runs once at boot, 30 s after WiFi connects
        xTaskCreate([](void *)
                    {
            vTaskDelay(pdMS_TO_TICKS(30000));
            printf("Checking for firmware update...\r\n");
            ota_service_check_and_update(); // reboots if newer firmware found
            vTaskDelete(nullptr); }, "ota_check", 8192, nullptr, 3, nullptr);
    }
    else
    {
        ESP_LOGW(TAG, "WiFi connection timeout, running without network");
        printf("WiFi: connection timeout, running without network\r\n");
        // Still try to initialise time so TZ is set correctly
        time_service_init();
    }

    transition(DeviceState::RUNNING);
    printf("Status: RUNNING – LED display active on GPIO 10\r\n");
    render_loop(cfg);
}
