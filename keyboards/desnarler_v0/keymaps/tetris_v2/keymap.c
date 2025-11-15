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
    // Initialize OS Switch
    setPinInputHigh(OS_SWITCH_PIN);

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
    writePinLow(LED2_PIN);
}

// ----------------------
// Called when layer changes
// ----------------------
layer_state_t layer_state_set_user(layer_state_t state) {
    state = update_tri_layer_state(state, 1, 2, 3);

    uint8_t layer = get_highest_layer(state);

    // LEDs
    writePinLow(LED1_PIN);
    writePinLow(LED2_PIN);
    writePinLow(LED3_PIN);

    switch (layer) {
        case 0:
        case 4:
            writePinHigh(LED1_PIN);
            break;
        case 1:
        case 5:
            writePinHigh(LED2_PIN);
            break;
        case 2:
        case 6:
            writePinHigh(LED3_PIN);
            break;
        case 3:
        case 7:
            writePinHigh(LED1_PIN);
            writePinHigh(LED2_PIN);
            writePinHigh(LED3_PIN);
            break;
    }

    return state;
}

// ----------------------
// matrix_scan_user: repeated loop
// ----------------------
void matrix_scan_user(void) {
    static int16_t last_raw = 0;           // previous slider reading
    static uint32_t last_time = 0;         // cooldown timer

    int16_t raw = analogReadPin(SLIDER_PIN);

    // --- compute relative change ---
    int16_t delta = raw - last_raw;
    last_raw = raw;

    // --- noise rejection ---
    if (abs(delta) < SLIDER_DEADBAND * 5) return; // adjust multiplier if needed

    // --- cooldown so game catches presses ---
    if (timer_elapsed32(last_time) < 80) { // 80ms ≈ 12 presses/s
        return;
    }

    // --- convert slider movement to arrow keys ---
    if (delta > 0) {
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
