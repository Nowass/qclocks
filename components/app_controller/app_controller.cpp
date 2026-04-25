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

// Define the QCLOCKS event base (declared in app_events.hpp).
ESP_EVENT_DEFINE_BASE(QCLOCKS_EVENTS);

static const char *TAG = "app_ctrl";
static DeviceState s_state = DeviceState::BOOT;

DeviceState app_controller_get_state(void)
{
    return s_state;
}

// ---------------------------------------------------------------------------
// Phase 1a stub boot flow (spec section 21).
// Real implementation added incrementally in Phases 1b-2.
// ---------------------------------------------------------------------------
void app_controller_start(void)
{
    ESP_LOGI(TAG, "qclocks starting – Phase 1a skeleton");

    settings_init();
    diagnostics_service_init();
    led_renderer_init();

    AppSettings cfg{};
    settings_load(&cfg);
    ESP_LOGI(TAG, "timezone: %s  brightness day/night: %u/%u",
             cfg.timezone, cfg.brightness_day, cfg.brightness_night);

    led_renderer_set_brightness(cfg.brightness_day);

    s_state = DeviceState::RUNNING;

    // Minimal render loop: logs the current time tokens every second.
    // Replace with real time once time_service is wired (Phase 2).
    uint8_t fake_hour   = 7;
    uint8_t fake_minute = 0;

    while (true) {
        auto tokens = render_time_tokens(fake_hour, fake_minute);
        led_renderer_set_tokens(tokens);
        led_renderer_show();

        fake_minute++;
        if (fake_minute >= 60) {
            fake_minute = 0;
            fake_hour = (fake_hour % 23) + 1;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
