#include "diagnostics_service.hpp"
#include "app_events.hpp"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_idf_version.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include <cstring>
#include <cstdio>

static const char *TAG = "diagnostics";
static DiagnosticsMode s_mode = DiagnosticsMode::NONE;

static const char* reset_reason_str(esp_reset_reason_t r)
{
    switch (r) {
        case ESP_RST_POWERON:  return "power-on";
        case ESP_RST_SW:       return "software";
        case ESP_RST_PANIC:    return "panic/exception";
        case ESP_RST_INT_WDT:  return "interrupt watchdog";
        case ESP_RST_TASK_WDT: return "task watchdog";
        case ESP_RST_BROWNOUT: return "brownout";
        default:               return "other";
    }
}

void diagnostics_service_print_info(void)
{
    int uptime_s = (int)(esp_timer_get_time() / 1000000);
    printf("\r\n=== qclocks diagnostics ===\r\n");
    printf("IDF version : %s\r\n", esp_get_idf_version());
    printf("Uptime      : %ds\r\n", uptime_s);
    printf("Free heap   : %" PRIu32 " bytes\r\n", esp_get_free_heap_size());
    printf("Reset reason: %s\r\n", reset_reason_str(esp_reset_reason()));
    printf("===========================\r\n\r\n");
    fflush(stdout);
}

static void uart_cmd_task(void *arg)
{
    char line[64];
    printf("\r\nqclocks console ready. Commands: info | ota | reset_wifi\r\n");
    fflush(stdout);

    while (true) {
        int i = 0;
        while (i < (int)sizeof(line) - 1) {
            int c = getchar();
            if (c == EOF || c == '\n' || c == '\r') {
                break;
            }
            if (c >= 0) {
                line[i++] = (char)c;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        line[i] = '\0';

        if (i == 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        while (i > 0 && (line[i-1] == ' ' || line[i-1] == '\r')) {
            line[--i] = '\0';
        }

        if (strcmp(line, "info") == 0) {
            diagnostics_service_print_info();
        } else if (strcmp(line, "ota") == 0) {
            printf("Triggering OTA update...\r\n"); fflush(stdout);
            esp_event_post(QCLOCKS_EVENTS, (int32_t)QclocksEvent::OTA_REQUESTED,
                           nullptr, 0, 0);
        } else if (strcmp(line, "reset_wifi") == 0) {
            ESP_LOGW(TAG, "Erasing WiFi credentials – device will reboot");
            nvs_handle_t h;
            if (nvs_open("wifi_creds", NVS_READWRITE, &h) == ESP_OK) {
                nvs_erase_all(h);
                nvs_commit(h);
                nvs_close(h);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        } else {
            ESP_LOGW(TAG, "Unknown command '%s'. Try: info, ota, reset_wifi", line);
        }
    }
}

bool diagnostics_service_init(void)
{
    ESP_LOGI(TAG, "diagnostics init");
    diagnostics_service_print_info();
    xTaskCreate(uart_cmd_task, "uart_cmd", 2048, nullptr, 2, nullptr);
    return true;
}

void diagnostics_service_set_mode(DiagnosticsMode mode)
{
    s_mode = mode;
}

DiagnosticsMode diagnostics_service_get_mode(void)
{
    return s_mode;
}
