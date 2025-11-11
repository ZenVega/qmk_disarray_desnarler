// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
//
#include "analog.h"
#include QMK_KEYBOARD_H

#define SLIDER_PIN 26
#define SLIDER_SENSITIVITY 12

static int16_t last_val = -1;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┐
     * │ 1 │ 2 │ 3 │
     * ├───┼───┼───┤
     * │ 5 │ 6 │ 7 │
     * └───┴───┴───┘
     */
    [0] = LAYOUT(KC_1, KC_2, 
                KC_3, KC_4)}; // ROW2

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
