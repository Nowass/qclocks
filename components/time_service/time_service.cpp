#include "time_service.hpp"
#include "settings_store.hpp"
#include "app_events.hpp"
#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_event.h"
#include <ctime>

static volatile bool s_time_valid = false;
static const char *TAG = "time_svc";

static void sntp_sync_callback(struct timeval *tv)
{
    (void)tv;
    s_time_valid = true;
    ESP_LOGI(TAG, "SNTP sync complete");
    esp_event_post(QCLOCKS_EVENTS, (int32_t)QclocksEvent::TIME_SYNCED, nullptr, 0, 0);
}

bool time_service_init(void)
{
    AppSettings cfg{};
    settings_load(&cfg);

    setenv("TZ", cfg.timezone, 1);
    tzset();
    ESP_LOGI(TAG, "TZ set to: %s", cfg.timezone);

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.cloudflare.com");
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    esp_sntp_set_time_sync_notification_cb(sntp_sync_callback);
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP initialized");

    return true;
}

bool time_service_is_valid(void)
{
    return s_time_valid;
}

bool time_service_get_local_tm(struct tm *out_tm)
{
    if (!out_tm || !s_time_valid) {
        return false;
    }
    time_t now = time(nullptr);
    localtime_r(&now, out_tm);
    return true;
}

time_t time_service_get_epoch(void)
{
    if (!s_time_valid) {
        return 0;
    }
    return time(nullptr);
}
