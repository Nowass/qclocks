#include "led_renderer.hpp"
#include "word_layout.hpp"
#include "led_index_map.hpp"
#include "esp_log.h"

// ---------------------------------------------------------------------------
// Phase 1d stub: UART backend
// Prints the active word tokens to the serial log instead of driving LEDs.
// Replace this file's body in Phase 4 with the WS2812B RMT backend.
// ---------------------------------------------------------------------------

static const char *TAG = "led_renderer";
static uint8_t s_brightness = 96;

bool led_renderer_init(void)
{
    ESP_LOGI(TAG, "LED renderer initialised (UART stub backend)");
    return true;
}

void led_renderer_clear(void)
{
    // nothing to clear in UART mode
}

void led_renderer_set_tokens(std::span<const TokenId> tokens)
{
    // In Phase 1 we just log the active tokens; the matrix visualisation
    // and real LED driving come in Phase 4.
    ESP_LOGI(TAG, "--- active tokens (brightness=%u) ---", s_brightness);
    for (TokenId tok : tokens) {
        auto cells = token_to_cells(tok);
        ESP_LOGI(TAG, "  token %u  (%u cells)", static_cast<unsigned>(tok), static_cast<unsigned>(cells.size()));
    }
}

void led_renderer_show(void)
{
    // nothing extra needed for UART mode
}

void led_renderer_set_brightness(uint8_t value)
{
    s_brightness = value;
}
