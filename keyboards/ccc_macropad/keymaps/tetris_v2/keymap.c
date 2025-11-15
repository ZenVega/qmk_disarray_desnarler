// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "analog.h"
#include QMK_KEYBOARD_H

// ----------------------
// Slider setup
// ----------------------
#define SLIDER_PIN 26
#define MAX_VOLUME_STEPS 20
#define SLIDER_DEADBAND 2 // ignore <2 steps of change
static bool     volume_init_done = false;
static bool     slider_ready     = false;
static uint32_t slider_timer     = 0;
static uint32_t init_timer       = 0;
static int16_t  last_val         = 0;

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
static bool switch_on = false;

// ----------------------
// helpers to hold tab
// ----------------------
static bool     gui_held      = false;
static uint32_t last_tab_time = 0;
#define GUI_HOLD_TIMEOUT 1000 // ms

// ----------------------
// Layer definitions
// ----------------------
#define BASE_LAYER_1 0
#define BASE_LAYER_2 4

// ----------------------
// Keymap
// ----------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // --- Linux layers 0–4 ---
    [0] = LAYOUT(KC_ENTER, KC_DOWN, KC_X, KC_ENTER),
    [1] = LAYOUT(_______, MO(2), LGUI(LSFT(LALT(KC_LEFT))), LGUI(LSFT(LALT(KC_RIGHT)))),
    [2] = LAYOUT(MO(1), _______, LGUI(KC_TAB), LGUI(LSFT(KC_TAB))),
    [3] = LAYOUT(_______, _______, KC_SYSTEM_SLEEP, KC_SYSTEM_SLEEP),

    // --- macOS layers 4–7 ---
    [4] = LAYOUT(KC_ENTER, KC_DOWN, KC_X, KC_ENTER),
    [5] = LAYOUT(_______, MO(6), LCTL(LSFT(KC_LEFT)), LCTL(LSFT(KC_RIGHT))),
    [6] = LAYOUT(MO(5), _______, LGUI(KC_TAB), LGUI(LSFT(KC_TAB))),
    [7] = LAYOUT(_______, _______, KC_SYSTEM_SLEEP, KC_SYSTEM_SLEEP)};

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
    // ------ OS_Switch: select OS layer set ------
    bool new_mode = !readPin(OS_SWITCH_PIN); // HIGH = macOS, LOW = Linux
    if (new_mode != switch_on) {
        switch_on           = new_mode;
        uint8_t target_base = switch_on ? BASE_LAYER_2 : BASE_LAYER_1;
        layer_move(target_base); // activate the correct OS base layer
    }

    // ------ GUI hold timeout ------
    if (gui_held && timer_elapsed32(last_tab_time) > GUI_HOLD_TIMEOUT) {
        unregister_mods(MOD_BIT(KC_LGUI));
        gui_held = false;
    }

    // ------ volume init ------
    if (!volume_init_done) {
        if (!init_timer) init_timer = timer_read32();
        if (timer_elapsed32(init_timer) > 800) {
            for (int i = 0; i < 50; i++)
                tap_code_delay(KC_VOLD, 5);
            last_val         = 0;
            volume_init_done = true;
            slider_timer     = timer_read32();
        }
        return;
    }

    // ------ slider ready delay ------
    if (!slider_ready) {
        if (timer_elapsed32(slider_timer) > 500)
            slider_ready = true;
        else
            return;
    }

    // ------ slider processing ------
    int16_t raw    = analogReadPin(SLIDER_PIN);
    int     target = (int)(raw * MAX_VOLUME_STEPS / 4095.0f);
    if (target < 0) target = 0;
    if (target > MAX_VOLUME_STEPS) target = MAX_VOLUME_STEPS;

    if (abs(target - last_val) <= SLIDER_DEADBAND) return; // noise filter

    if (target > last_val) {
        for (int i = last_val; i < target; i++)
            tap_code(KC_RIGHT);
    } else {
        for (int i = target; i < last_val; i++)
            tap_code(KC_LEFT);
    }
    last_val = target;
}

// ----------------------
// Called on every keypress
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    // Sleep animation
    if (keycode == KC_SYSTEM_SLEEP) {
        for (uint8_t i = 0; i < 5; i++) {
            writePinHigh(LED1_PIN);
            writePinHigh(LED2_PIN);
            writePinHigh(LED3_PIN);
            wait_ms(200);
            writePinLow(LED1_PIN);
            writePinLow(LED2_PIN);
            writePinLow(LED3_PIN);
            wait_ms(200);
        }
        return false;
    }

    // Custom keys
    switch (keycode) {
        // holding tab after pressed
        case LGUI(KC_TAB):
        case LGUI(LSFT(KC_TAB)): {
            if (!gui_held) {
                register_mods(MOD_BIT(KC_LGUI)); // hold LGUI
                gui_held = true;
            }

            if (keycode == LGUI(KC_TAB))
                tap_code(KC_TAB); // forward
            else {
                register_mods(MOD_BIT(KC_LSFT)); // hold Shift
                tap_code(KC_TAB);
                unregister_mods(MOD_BIT(KC_LSFT));
            }
            last_tab_time = timer_read32();
            return false;
        }
    }
    return true;
}
