#pragma once

#define DYNAMIC_KEYMAP_LAYER_COUNT 8

// rgblight — WS2812 strip on GP29
#define RGBLIGHT_DI_PIN GP29
#define RGBLIGHT_LIMIT_VAL 200
#define RGBLIGHT_HUE_STEP 8
#define RGBLIGHT_SAT_STEP 17
#define RGBLIGHT_VAL_STEP 17
#define RGBLIGHT_SLEEP

// Extra GPIO broken out on the board, unused by the matrix/encoders/lighting
// above. Named here so a keymap can reference them (e.g. writePin(), or an
// i2c1/analog QMK driver) instead of bare GPxx numbers. Delete whichever you
// don't use.
#define MACRO_GOAT_PIN_14 GP14 // SPI SCK / I2C1 SDA
#define MACRO_GOAT_PIN_15 GP15 // UART TX / I2C1 SCL
#define MACRO_GOAT_PIN_26 GP26 // ADC0 / I2C SDA
#define MACRO_GOAT_PIN_27 GP27 // ADC1 / I2C SCL
#define MACRO_GOAT_PIN_28 GP28 // ADC2
