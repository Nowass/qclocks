#pragma once

#include <cstdint>

static constexpr uint8_t  MATRIX_ROWS = 8;
static constexpr uint8_t  MATRIX_COLS = 20;
static constexpr uint16_t LED_COUNT   = MATRIX_ROWS * MATRIX_COLS;

// GPIO pin for WS2812B data line.
// Change this single constant to match your PCB wiring.
static constexpr int WS2812B_DATA_GPIO = 10;

struct CellCoord
{
    uint8_t row;
    uint8_t col;
};
