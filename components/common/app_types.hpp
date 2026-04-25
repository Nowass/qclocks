#pragma once

#include <cstdint>

static constexpr uint8_t MATRIX_ROWS = 8;
static constexpr uint8_t MATRIX_COLS = 20;
static constexpr uint16_t LED_COUNT = MATRIX_ROWS * MATRIX_COLS;

struct CellCoord
{
    uint8_t row;
    uint8_t col;
};
