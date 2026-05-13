#pragma once

#include <cstdint>

// Boot animation phases.
// The caller advances the phase as services become ready.
enum class BootPhase : uint8_t
{
    WIFI_CONNECTING, // scanning row sweeps down – waiting for WiFi
    TIME_SYNCING,    // word reveal loop – WiFi ok, waiting for SNTP
    DONE,            // all-blink + fade out – time is ready
};

// Start the boot animation task (call once after led_renderer_init).
void led_boot_animation_start(void);

// Advance to the next phase (thread-safe, can be called from any task).
void led_boot_animation_set_phase(BootPhase phase);

// Stop the boot animation task and clear the display.
// Must be called before entering the normal render_loop.
void led_boot_animation_stop(void);
