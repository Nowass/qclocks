#pragma once

// Initialise the OTA service.
bool ota_service_init(void);

// Check the configured server for a newer firmware version.
bool ota_service_check_for_update(void);

// Download and apply the new firmware (rollback-safe).
// Reboots on success.
bool ota_service_start_update(void);
