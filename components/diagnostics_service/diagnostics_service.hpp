#pragma once

#include <cstdint>

// Diagnostic / test modes (spec section 16.1).
enum class DiagnosticsMode : uint8_t {
    NONE              = 0,
    LED_ALL_ON        = 1,
    LED_ROW_SCAN      = 2,
    LED_COL_SCAN      = 3,
    WORD_SCAN         = 4,
    MATRIX_INDEX_SCAN = 5,
};

// Initialise the diagnostics service.
bool diagnostics_service_init(void);

// Print runtime info to UART (spec section 16.2).
void diagnostics_service_print_info(void);

// Set / get the current diagnostic display mode.
void diagnostics_service_set_mode(DiagnosticsMode mode);
DiagnosticsMode diagnostics_service_get_mode(void);
