#pragma once

// Initialise the OTA service.
bool ota_service_init(void);

// Check the configured server for a newer firmware version.
bool ota_service_check_for_update(void);

// Download and apply the new firmware (rollback-safe).
// Reboots on success.
bool ota_service_start_update(void);

// Check version on server; if newer, download and apply immediately.
// Returns false if same version or on error. Reboots on successful update.
bool ota_service_check_and_update(void);
