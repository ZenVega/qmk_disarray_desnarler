// Companion keymap — keys send F13–F16 to the macOS companion app.
#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_F13, KC_F14, KC_F15, KC_F16),
    [1] = LAYOUT(KC_F13, KC_F14, KC_F15, KC_F16),
};
