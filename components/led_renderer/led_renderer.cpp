#include "led_renderer.hpp"
#include "word_layout.hpp"
#include "app_types.hpp"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"

#include "led_index_map.hpp"
#include "freertos/FreeRTOS.h"

static const char *TAG = "led_renderer";

// ---------------------------------------------------------------------------
// Pixel buffer: GRB order as required by WS2812B, 3 bytes per LED.
// ---------------------------------------------------------------------------
static uint8_t s_pixels[LED_COUNT * 3];
static uint8_t s_brightness = 96;

static rmt_channel_handle_t s_rmt_chan = nullptr;
static rmt_encoder_handle_t s_encoder  = nullptr;

// WS2812B timing at 10 MHz RMT resolution (1 tick = 100 ns).
// Tolerances per datasheet: T0H 400±150 ns, T1H 800±150 ns.
static constexpr uint32_t RMT_RESOLUTION_HZ = 10'000'000;
static constexpr uint16_t T0H = 4;   // 400 ns
static constexpr uint16_t T0L = 9;   // 900 ns  (spec ≥850 ns)
static constexpr uint16_t T1H = 8;   // 800 ns
static constexpr uint16_t T1L = 5;   // 500 ns  (spec ≥450 ns)

static const rmt_transmit_config_t s_tx_cfg = {
    .loop_count = 0,
    .flags      = { .eot_level = 0, .queue_nonblocking = 0 },
};

// ---------------------------------------------------------------------------
bool led_renderer_init(void)
{
    rmt_tx_channel_config_t chan_cfg = {};
    chan_cfg.gpio_num           = static_cast<gpio_num_t>(WS2812B_DATA_GPIO);
    chan_cfg.clk_src            = RMT_CLK_SRC_DEFAULT;
    chan_cfg.resolution_hz      = RMT_RESOLUTION_HZ;
    chan_cfg.mem_block_symbols  = 64;   // 64 RMT symbols in HW memory
    chan_cfg.trans_queue_depth  = 4;

    if (rmt_new_tx_channel(&chan_cfg, &s_rmt_chan) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel on GPIO %d", WS2812B_DATA_GPIO);
        return false;
    }

    rmt_bytes_encoder_config_t enc_cfg = {};
    enc_cfg.bit0.level0    = 1; enc_cfg.bit0.duration0 = T0H;
    enc_cfg.bit0.level1    = 0; enc_cfg.bit0.duration1 = T0L;
    enc_cfg.bit1.level0    = 1; enc_cfg.bit1.duration0 = T1H;
    enc_cfg.bit1.level1    = 0; enc_cfg.bit1.duration1 = T1L;
    enc_cfg.flags.msb_first = 1;  // WS2812B expects MSB first

    if (rmt_new_bytes_encoder(&enc_cfg, &s_encoder) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT bytes encoder");
        return false;
    }

    rmt_enable(s_rmt_chan);
    memset(s_pixels, 0, sizeof(s_pixels));

    ESP_LOGI(TAG, "WS2812B RMT driver ready on GPIO %d (%u LEDs)", WS2812B_DATA_GPIO, LED_COUNT);
    return true;
}

// ---------------------------------------------------------------------------
// Set a single pixel (brightness-scaled, GRB storage).
// ---------------------------------------------------------------------------
static void set_pixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b)
{
    if (idx >= LED_COUNT) return;
    uint16_t sc = s_brightness;
    s_pixels[idx * 3 + 0] = static_cast<uint8_t>((uint16_t)g * sc / 255u);
    s_pixels[idx * 3 + 1] = static_cast<uint8_t>((uint16_t)r * sc / 255u);
    s_pixels[idx * 3 + 2] = static_cast<uint8_t>((uint16_t)b * sc / 255u);
}

// ---------------------------------------------------------------------------
void led_renderer_clear(void)
{
    memset(s_pixels, 0, sizeof(s_pixels));
}

void led_renderer_set_tokens(std::span<const TokenId> tokens)
{
    memset(s_pixels, 0, sizeof(s_pixels));
    for (TokenId tok : tokens) {
        auto cells = token_to_cells(tok);
        for (const CellCoord &c : cells) {
            uint16_t idx = matrix_to_led_index(c.row, c.col);
            set_pixel(idx, 255, 220, 180);  // warm white
        }
    }
}

void led_renderer_show(void)
{
    // Ensure previous frame finished before sending next
    rmt_tx_wait_all_done(s_rmt_chan, portMAX_DELAY);
    rmt_transmit(s_rmt_chan, s_encoder, s_pixels, sizeof(s_pixels), &s_tx_cfg);
    // Reset pulse (>50 µs low) is guaranteed by the ≥5 s delay in render_loop.
}

void led_renderer_set_brightness(uint8_t value)
{
    s_brightness = value;
}

