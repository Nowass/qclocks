#pragma once

#include "settings_model.hpp"

// Initialise NVS and load settings namespace.
bool settings_init(void);

// Load settings from NVS into *out. Writes defaults on first boot.
bool settings_load(AppSettings *out);

// Persist *in to NVS.
bool settings_save(const AppSettings *in);

// Erase settings namespace (reverts to defaults on next boot).
bool settings_reset(void);
