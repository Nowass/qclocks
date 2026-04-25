#pragma once

#include "esp_event.h"

// Event base for all qclocks application events
ESP_EVENT_DECLARE_BASE(QCLOCKS_EVENTS);

enum class QclocksEvent : int32_t
{
    WIFI_CONNECTED = 0,
    WIFI_DISCONNECTED = 1,
    TIME_SYNCED = 2,
    PROVISIONING_DONE = 3,
    OTA_REQUESTED = 4,
};
