#pragma once

// Initialise the Wi-Fi driver and event handlers.
bool wifi_service_init(void);

// Attempt to connect using stored credentials.
bool wifi_service_connect(void);

// Returns true if currently associated to an AP.
bool wifi_service_is_connected(void);
