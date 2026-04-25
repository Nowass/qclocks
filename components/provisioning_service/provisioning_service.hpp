#pragma once

// Initialise the provisioning service.
bool provisioning_service_init(void);

// Returns true if Wi-Fi credentials are stored in NVS.
bool provisioning_service_has_credentials(void);

// Start SoftAP provisioning mode (blocks until credentials are saved).
bool provisioning_service_start(void);

// Erase stored Wi-Fi credentials and force next boot into provisioning.
bool provisioning_service_reset(void);
