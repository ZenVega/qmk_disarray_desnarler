// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "analog.h"
#include QMK_KEYBOARD_H

// ----------------------
// Slider setup
// ----------------------
#define SLIDER_PIN 29

static uint32_t last_vol_change = 0;
static bool slider_ready = false;
const int16_t center = 512;
const int16_t dead_zone = 70;

// ----------------------
// LEDs
// ----------------------
#define LED1_PIN 26 // left LED
#define LED2_PIN 28
#define LED3_PIN 27 // right LED

// ----------------------
// Switch
// ----------------------
#define OS_SWITCH_PIN GP0
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
// custom keys
// ----------------------
enum custom_keycodes {
    AUML = SAFE_RANGE,
    OUML,
    UUML,
    DSCHAR,
    CCAP
};

static bool capitalize = false;

// ----------------------
// Keymap
// ----------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // --- Linux layers 0–4 ---
    [0] = LAYOUT(MO(1), MO(2), LGUI(LALT(KC_LEFT)), LGUI(LALT(KC_RIGHT))),
    [1] = LAYOUT(MO(1), MO(2), LGUI(LSFT(LALT(KC_LEFT))), LGUI(LSFT(LALT(KC_RIGHT)))),
    [2] = LAYOUT(MO(1), _______, LGUI(KC_TAB), LGUI(LSFT(KC_TAB))),
    [3] = LAYOUT(_______, _______, KC_SYSTEM_SLEEP, KC_SYSTEM_SLEEP),

    // --- Umlaut Layer
    [4] = LAYOUT(AUML, OUML, UUML, DSCHAR)
};


void initial_blink(void){
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
// Called once at boot
// ----------------------
void matrix_init_user(void) {
    // Initialize OS Switch
    setPinInputHigh(OS_SWITCH_PIN);

    // Initialize LEDs
    setPinOutput(LED1_PIN);
    setPinOutput(LED2_PIN);
    setPinOutput(LED3_PIN);

    initial_blink();
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
            writePinHigh(LED1_PIN);
            break;
        case 1:
            writePinHigh(LED2_PIN);
            break;
        case 2:
            writePinHigh(LED3_PIN);
            break;
        case 3:
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
    
    // ------ Switch: select layer set ------
    bool new_mode = !readPin(OS_SWITCH_PIN);
    if (new_mode != switch_on) {
        switch_on           = new_mode;
        uint8_t target_base = switch_on ? BASE_LAYER_2 : BASE_LAYER_1;
        layer_move(target_base);
    }

    if (switch_on){
        writePinHigh(LED1_PIN);
        writePinLow(LED2_PIN);
        writePinHigh(LED3_PIN);
    }

    // ------ GUI hold timeout ------
    if (gui_held && timer_elapsed32(last_tab_time) > GUI_HOLD_TIMEOUT) {
        unregister_mods(MOD_BIT(KC_LGUI));
        gui_held = false;
    }

    // ------ start slider once ------
    if (!slider_ready){
        last_vol_change = timer_read32();
        slider_ready = true;
        return ;
    }

    // ------ read slider --------
    int16_t raw = analogReadPin(SLIDER_PIN);
    if (timer_elapsed32(last_vol_change) < 100){
        return ;
    }
   
    if (raw < center - dead_zone){
        tap_code(KC_AUDIO_VOL_DOWN);
        last_vol_change = timer_read32();
    }
    else if (raw > center + dead_zone){
        tap_code(KC_AUDIO_VOL_UP);
        last_vol_change = timer_read32();
    }
}

void    umlaut_pressed(void){
    writePinLow(LED1_PIN);
    writePinHigh(LED2_PIN);
    writePinLow(LED3_PIN);
    wait_ms(100);
}

// ----------------------
// Called on every keypress
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    
    if (!record->event.pressed){
        capitalize = false;
        return false;
    }

    // typing the key combination step-by-step
    switch (keycode) {
        case CCAP:{
            capitalize = true;
            return false;
        }
        case AUML: {
            tap_code(KC_RALT);
            register_code(KC_LSFT);
            tap_code(KC_QUOT);
            unregister_code(KC_LSFT);
            if (capitalize)
                register_code(KC_LSFT);
            tap_code(KC_A);
            if (capitalize)
                unregister_code(KC_LSFT);
            umlaut_pressed();
            
            return false;
        }
        case OUML: {
            tap_code(KC_RALT);
            register_code(KC_LSFT);
            tap_code(KC_QUOT);
            unregister_code(KC_LSFT);
            if (capitalize)
                register_code(KC_LSFT);
            tap_code(KC_O);
            if (capitalize)
                unregister_code(KC_LSFT);
            umlaut_pressed();
            
            return false;
        }
        case UUML: {
            tap_code(KC_RALT);
            register_code(KC_LSFT);
            tap_code(KC_QUOT);
            unregister_code(KC_LSFT);
            if (capitalize)
                register_code(KC_LSFT);
            tap_code(KC_U);
            if (capitalize)
                unregister_code(KC_LSFT);
            umlaut_pressed();
            
            return false;
        }
        case DSCHAR: {
            tap_code(KC_RALT);
            tap_code(KC_S);
            tap_code(KC_S);
            umlaut_pressed();
            
            return false;
        }
    }
    
    // holding tab
    switch (keycode) {
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
