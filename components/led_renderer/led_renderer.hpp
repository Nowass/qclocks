#pragma once

#include "word_tokens.hpp"
#include <cstdint>
#include <span>

// Initialise the renderer backend (UART stub for Phase 1, WS2812B for Phase 4).
bool led_renderer_init(void);

// Clear all LEDs (set to off).
void led_renderer_clear(void);

// Activate the cells corresponding to the given token set.
void led_renderer_set_tokens(std::span<const TokenId> tokens);

// Push the current buffer to the physical output (UART / WS2812B).
void led_renderer_show(void);

// Set global brightness (0-255).
void led_renderer_set_brightness(uint8_t value);

// Set a single LED by physical index (raw RGB, not brightness-scaled).
// Intended for boot animations in led_effects.
void led_renderer_set_pixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
