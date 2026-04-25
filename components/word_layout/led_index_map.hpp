#pragma once

#include "app_types.hpp"
#include <cstdint>

// Serpentine (snake) wiring layout:
//   even rows: left-to-right  -> index = row * COLS + col
//   odd  rows: right-to-left  -> index = row * COLS + (COLS - 1 - col)
uint16_t matrix_to_led_index(uint8_t row, uint8_t col);

// Explicit static lookup table [row][col] -> LED index.
// Generated from the serpentine rule but can be overridden for mechanical fixes.
extern const uint16_t led_index_map[MATRIX_ROWS][MATRIX_COLS];
