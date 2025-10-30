// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
//
#include "analog.h"
#include QMK_KEYBOARD_H

#define SLIDER_PIN 26
#define SLIDER_SENSITIVITY 12
#define LED1_PIN 29 // left LED
#define LED2_PIN 27
#define LED3_PIN 28 // right LED
#define OS_SWITCH_PIN GP3

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* Layer 0: Workspace switching */
    [0] = LAYOUT(MO(1), MO(2), KC_WS_LEFT, KC_WS_RIGHT),

    /* Layer 1: Move window between workspaces */
    [1] = LAYOUT(MO(1), MO(2), KC_MOVE_LEFT, KC_MOVE_RIGHT),

    /* Layer 2: Switch windows */
    [2] = LAYOUT(MO(1), MO(2), LGUI(KC_TAB), LGUI(LSFT(KC_TAB)))};

void matrix_init_user(void) {
    // Initialize OS Switch
    setPinInputHigh(OS_SWITCH_PIN);

    // Initialize LEDs
    setPinOutput(LED1_PIN);
    setPinOutput(LED2_PIN);
    setPinOutput(LED3_PIN);

    // initial happy blinking
    int i = 0;
    while (i < 10) {
        writePinHigh(LED1_PIN);
        writePinLow(LED2_PIN);
        writePinHigh(LED3_PIN);
        wait_ms(200);
        writePinLow(LED1_PIN);
        writePinHigh(LED2_PIN);
        writePinLow(LED3_PIN);
        wait_ms(200);
        i++;
    }
    writePinLow(LED2_PIN);
}

void matrix_scan_user(void) {
    int16_t slider_val = analogReadPin(SLIDER_PIN);

    // Map to a smaller range so it’s not too sensitive
    int16_t step = slider_val / SLIDER_SENSITIVITY;

    // compare with last value and call (VOLUME_UP/VOLUME_DOWN) on change;
    if (step != last_val) {
        if (step > last_val) {
            tap_code(KC_VOLU);
        } else {
            tap_code(KC_VOLD);
        }
        last_val = step;
    }
}
