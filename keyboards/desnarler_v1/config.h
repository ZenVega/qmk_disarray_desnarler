// keyboards/ccc_macropad/config.h
#pragma once

#define DYNAMIC_KEYMAP_LAYER_COUNT 8

// RGB Matrix (RP2040-safe)
#define WS2812_DI_PIN GP16    // still the internal RGB
#define RGBLED_NUM 1       // only 1 internal LED
#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_STARTUP_MODE RGB_MATRIX_CYCLE_ALL  // slow rainbow

