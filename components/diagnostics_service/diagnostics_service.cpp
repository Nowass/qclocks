#include "diagnostics_service.hpp"
#include "esp_log.h"

static const char *TAG = "diagnostics";
static DiagnosticsMode s_mode = DiagnosticsMode::NONE;

bool diagnostics_service_init(void) { return true; }

void diagnostics_service_print_info(void)
{
    ESP_LOGI(TAG, "qclocks diagnostics stub – Phase 1a");
}

void diagnostics_service_set_mode(DiagnosticsMode mode)
{
    s_mode = mode;
}

DiagnosticsMode diagnostics_service_get_mode(void)
{
    return s_mode;
}
