#pragma once

#include <cstdint>
#include <cstring>

// Persistent application settings stored in NVS (spec section 15.2).
struct AppSettings {
    uint8_t  brightness_day;
    uint8_t  brightness_night;
    bool     night_mode_enabled;
    uint8_t  night_start_hour;
    uint8_t  night_start_minute;
    uint8_t  night_end_hour;
    uint8_t  night_end_minute;
    char     timezone[64];
    char     ota_url[192];
    char     hostname[64];
};
