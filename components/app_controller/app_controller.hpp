#pragma once

#include <cstdint>

// Runtime states of the device (spec section 10.1).
enum class DeviceState : uint8_t {
    BOOT                = 0,
    NEEDS_PROVISIONING  = 1,
    CONNECTING_WIFI     = 2,
    ONLINE_SYNCING_TIME = 3,
    RUNNING             = 4,
    OTA_IN_PROGRESS     = 5,
    ERROR               = 6,
};

// Entry point called from app_main.
void app_controller_start(void);

// Returns the current device state.
DeviceState app_controller_get_state(void);
