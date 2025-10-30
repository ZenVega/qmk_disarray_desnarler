// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
//

#include "analog.h"
#include QMK_KEYBOARD_H

//-----
// slider setup
//-----
#define SLIDER_PIN 26
#define SLIDER_SENSITIVITY 12
static bool    volume_initialized = false;
static int16_t last_val           = 0;
#define MAX_VOLUME_STEPS 100

// static int16_t last_val = -1;

//----------
// LEDs
//----------

#define LED1_PIN 29 // left LED
#define LED2_PIN 27
#define LED3_PIN 28 // right LED

// ----------------------
// OS Switch
// ----------------------
#define OS_SWITCH_PIN GP3
static bool is_mac = false;

// ----------------------
// Custom keycodes
// ----------------------
enum custom_keys { KC_WS_LEFT, KC_WS_RIGHT, KC_MOVE_LEFT, KC_MOVE_RIGHT };

static inline int16_t smooth(int16_t new_val) {
    static int32_t filtered = 0;
    filtered                = (filtered * 9 + new_val) / 10; // 90% old, 10% new
    return (int16_t)filtered;
}

// ----------------------
// Keymap
// ----------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* Layer 0: Workspace switching */
    [0] = LAYOUT(MO(1), MO(2), KC_WS_LEFT, KC_WS_RIGHT),

    /* Layer 1: Move window between workspaces */
    [1] = LAYOUT(MO(1), MO(2), KC_MOVE_LEFT, KC_MOVE_RIGHT),

    /* Layer 2: Switch windows */
    [2] = LAYOUT(MO(1), MO(2), LGUI(KC_TAB), LGUI(LSFT(KC_TAB)))};

//--------------------
// called once at boot
//--------------------
void matrix_init_user(void) {
    // Initialize OS Switch
    setPinInputHigh(OS_SWITCH_PIN);

    // Initialize LEDs
    setPinOutput(LED1_PIN);
    setPinOutput(LED2_PIN);
    setPinOutput(LED3_PIN);

    // initialize volume
    if (!volume_initialized) {
        // set volume to zero
        for (int i = 0; i < 50; i++)
            tap_code(KC_VOLD);
        volume_initialized = true;
        last_val           = 0;
    }

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

// --------------------------
// called once after boot is done
// -------------------------
void post_init_user(void) {
    /*
        to have the intial blinking here does not work,
        checking out the layer state seems to immediately start after boot
    */
}

//-----------
// called when layer changes
// --------------
layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state);

    writePinLow(LED1_PIN);
    writePinLow(LED2_PIN);
    writePinLow(LED3_PIN);

    switch (layer) {
        case 0:
            writePinHigh(LED1_PIN);
            break;
        case 1:
            writePinHigh(LED2_PIN);
            break;
        case 2:
            writePinHigh(LED3_PIN);
            break;
    }
    return state;
}

// ----------------------
// repeated loop, constantly called while running
// ----------------------
void matrix_scan_user(void) {
    // ------ OS_Switch -------- //
    bool new_mode = !readPin(OS_SWITCH_PIN); // HIGH = macOS, LOW = Linux
    if (new_mode != is_mac) {
        is_mac = new_mode;
    }

    // ----- Slider ------ //
    if (!volume_initialized) return;

    int16_t raw    = analogReadPin(SLIDER_PIN);
    int     target = (int)(raw * MAX_VOLUME_STEPS / 4095.0f);

    // cut of edges
    if (target < 0) target = 0;
    if (target > MAX_VOLUME_STEPS) target = MAX_VOLUME_STEPS;

    // --- Apply volume changes ---
    if (target != last_val) {
        if (target > last_val) {
            for (int i = last_val; i < target; i++)
                tap_code(KC_VOLU);
        } else {
            for (int i = target; i < last_val; i++)
                tap_code(KC_VOLD);
        }
        last_val = target;
    }
}

// ----------------------
// Called on every keypress - Process custom keys
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // if nothing is pressed, there is no decision to be made
    if (!record->event.pressed) return true;

    // if pressed, decide what the meaning of the pressed keys should be
    switch (keycode) {
        case KC_WS_LEFT:
            tap_code16(is_mac ? LCTL(KC_LEFT) : LGUI(LALT(KC_LEFT)));
            return false;
        case KC_WS_RIGHT:
            tap_code16(is_mac ? LCTL(KC_RIGHT) : LGUI(LALT(KC_RIGHT)));
            return false;
        case KC_MOVE_LEFT:
            tap_code16(is_mac ? LCTL(LSFT(KC_LEFT)) : LGUI(LSFT(LALT(KC_LEFT))));
            return false;
        case KC_MOVE_RIGHT:
            tap_code16(is_mac ? LCTL(LSFT(KC_RIGHT)) : LGUI(LSFT(LALT(KC_RIGHT))));
            return false;
    }
    return true;
}
