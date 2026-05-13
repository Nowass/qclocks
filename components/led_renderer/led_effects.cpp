#include "led_effects.hpp"
#include "led_renderer.hpp"
#include "app_types.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ---------------------------------------------------------------------------
// Warm-white colour used for all boot animation pixels (matches normal display)
// ---------------------------------------------------------------------------
static constexpr uint8_t BW_R = 255;
static constexpr uint8_t BW_G = 220;
static constexpr uint8_t BW_B = 180;

// Dim colour for the "trailing" rows in the scanner effect
static constexpr uint8_t DIM_R = 40;
static constexpr uint8_t DIM_G = 35;
static constexpr uint8_t DIM_B = 28;

// ---------------------------------------------------------------------------
// Shared state
// ---------------------------------------------------------------------------
static volatile BootPhase s_phase = BootPhase::WIFI_CONNECTING;
static volatile bool s_stop = false;
static TaskHandle_t s_task_handle = nullptr;
static SemaphoreHandle_t s_mutex = nullptr;

// ---------------------------------------------------------------------------
// Helper: light up one full row
// ---------------------------------------------------------------------------
static void set_row(uint8_t row, uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t col = 0; col < MATRIX_COLS; ++col)
    {
        // Serpentine index (same formula as matrix_to_led_index)
        uint16_t idx = (row % 2 == 0)
                           ? static_cast<uint16_t>(row * MATRIX_COLS + col)
                           : static_cast<uint16_t>(row * MATRIX_COLS + (MATRIX_COLS - 1 - col));
        led_renderer_set_pixel(idx, r, g, b);
    }
}

// ---------------------------------------------------------------------------
// Phase 1 – row scanner (WiFi connecting)
//   A bright row sweeps top→bottom→top endlessly at ~280 ms/row.
//   One dim "trail" row follows behind.
// ---------------------------------------------------------------------------
static void phase_wifi(void)
{
    int8_t dir = 1;
    int8_t row = 0;

    while (!s_stop && s_phase == BootPhase::WIFI_CONNECTING)
    {
        led_renderer_clear();
        set_row(static_cast<uint8_t>(row), BW_R, BW_G, BW_B);

        // dim trail
        int8_t trail = row - dir;
        if (trail >= 0 && trail < MATRIX_ROWS)
        {
            set_row(static_cast<uint8_t>(trail), DIM_R, DIM_G, DIM_B);
        }

        led_renderer_show();
        vTaskDelay(pdMS_TO_TICKS(280));

        row += dir;
        if (row >= MATRIX_ROWS)
        {
            row = MATRIX_ROWS - 2;
            dir = -1;
        }
        if (row < 0)
        {
            row = 1;
            dir = 1;
        }
    }
}

// ---------------------------------------------------------------------------
// Phase 2 – word reveal (SNTP syncing)
//   Tokens from word_tokens are lit one by one cycling through all 31 tokens.
//   Each token stays on for 200 ms then fades to the next.
// ---------------------------------------------------------------------------
static void phase_syncing(void)
{
    // We light entire rows in sequence to keep it simple and avoid pulling in
    // word_layout here.  One row at a time, short on, then next.
    uint8_t row = 0;
    while (!s_stop && s_phase == BootPhase::TIME_SYNCING)
    {
        led_renderer_clear();
        set_row(row, BW_R, BW_G, BW_B);
        led_renderer_show();
        vTaskDelay(pdMS_TO_TICKS(150));

        row = (row + 1) % MATRIX_ROWS;
    }
}

// ---------------------------------------------------------------------------
// Phase 3 – all blink + fade out (transition to RUNNING)
// ---------------------------------------------------------------------------
static void phase_done(void)
{
    // 3× full-matrix blink
    for (int i = 0; i < 3 && !s_stop; ++i)
    {
        led_renderer_clear();
        for (uint16_t idx = 0; idx < LED_COUNT; ++idx)
        {
            led_renderer_set_pixel(idx, BW_R, BW_G, BW_B);
        }
        led_renderer_show();
        vTaskDelay(pdMS_TO_TICKS(120));

        led_renderer_clear();
        led_renderer_show();
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    // Fade out: step brightness from current down to 0
    uint8_t br = 96;
    while (br > 0 && !s_stop)
    {
        br = (br > 8) ? (br - 8) : 0;
        led_renderer_set_brightness(br);
        for (uint16_t idx = 0; idx < LED_COUNT; ++idx)
        {
            led_renderer_set_pixel(idx, BW_R, BW_G, BW_B);
        }
        led_renderer_show();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    led_renderer_clear();
    led_renderer_show();
}

// ---------------------------------------------------------------------------
// Animation task
// ---------------------------------------------------------------------------
static void boot_animation_task(void *)
{
    while (!s_stop)
    {
        switch (s_phase)
        {
        case BootPhase::WIFI_CONNECTING:
            phase_wifi();
            break;
        case BootPhase::TIME_SYNCING:
            phase_syncing();
            break;
        case BootPhase::DONE:
            phase_done();
            s_stop = true;
            break;
        }
    }
    led_renderer_clear();
    led_renderer_show();
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void led_boot_animation_start(void)
{
    s_stop = false;
    s_phase = BootPhase::WIFI_CONNECTING;
    s_mutex = xSemaphoreCreateMutex();
    xTaskCreate(boot_animation_task, "boot_anim", 2048, nullptr, 4, &s_task_handle);
}

void led_boot_animation_set_phase(BootPhase phase)
{
    s_phase = phase;
}

void led_boot_animation_stop(void)
{
    s_stop = true;
    // Give the task time to exit cleanly and clear the display
    vTaskDelay(pdMS_TO_TICKS(350));
    s_task_handle = nullptr;
}
