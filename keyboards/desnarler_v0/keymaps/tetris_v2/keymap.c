// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "analog.h"

// ----------------------
// Slider setup
// ----------------------
#define SLIDER_PIN GP26

#define SLIDER_DEADBAND 2 // ignore <2 steps of change

// ----------------------
// LEDs
// ----------------------
#define LED1_PIN 29 // left LED
#define LED2_PIN 27
#define LED3_PIN 28 // right LED

// ----------------------
// OS Switch
// ----------------------
#define OS_SWITCH_PIN GP3

// ----------------------
// Keymap
// ----------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_ENTER, KC_DOWN, KC_X, KC_ENTER
    ),
};

// ----------------------
// Called once at boot
// ----------------------
void matrix_init_user(void) {
    // Initialize LEDs
    setPinOutput(LED1_PIN);
    setPinOutput(LED2_PIN);
    setPinOutput(LED3_PIN);

    // initial happy blinking
    for (int i = 0; i < 10; i++) {
        writePinHigh(LED1_PIN);
        writePinLow(LED2_PIN);
        writePinHigh(LED3_PIN);
        wait_ms(200);
        writePinLow(LED1_PIN);
        writePinHigh(LED2_PIN);
        writePinLow(LED3_PIN);
        wait_ms(200);
    }
    //writePinLow(LED2_PIN);
}

// ----------------------
// matrix_scan_user: repeated loop
// ----------------------
void matrix_scan_user(void) {
    static int16_t smoothed = -1;
    static uint32_t last_time = 0;

    int16_t raw = analogReadPin(SLIDER_PIN);

    // First run
    if (smoothed < 0) {
        smoothed = raw;
        return;
    }

    //slider has 1024 positions theoretically
    const int16_t center = 512;     
    const int16_t deadzone = 50;    // adjust to taste

    int8_t direction = 0;
    if (smoothed < center - deadzone) {
        direction = -1;  // LEFT
    } else if (smoothed > center + deadzone) {
        direction = +1;  // RIGHT
    }

    // --- no press in center zone ---
    if (direction == 0) return;

    // --- rate limit key presses ---
    if (timer_elapsed32(last_time) < 50) {  // 20 keys/sec
        return;
    }

    if (direction > 0) {
        tap_code(KC_RIGHT);
    } else {
        tap_code(KC_LEFT);
    }
    last_time = timer_read32();
}

// ----------------------
// Called on every keypress
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed)
        return true;
    return true;
}
