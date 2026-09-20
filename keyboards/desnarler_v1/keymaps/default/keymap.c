// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "analog.h"
#include QMK_KEYBOARD_H

// ----------------------
// Slider setup
// ----------------------
#define SLIDER_PIN 29

// Volume follows fader *movement*: one volume tap per SLIDER_STEP of
// travel (1024 / 64 = 16 steps end to end). The step size doubles as
// hysteresis, so ADC noise never fires a tap and the volume stays put
// when the fader rests anywhere.
#define SLIDER_STEP 64

static uint32_t last_slider_read = 0;
static int16_t  slider_anchor = -1; // raw value at last tap, -1 = uninitialized

// ----------------------
// LEDs
// ----------------------
#define LED1_PIN 26 // left LED
#define LED2_PIN 28
#define LED3_PIN 27 // right LED

// ----------------------
// Switch
// ----------------------
#define LAYER_SWITCH_PIN GP0
static bool switch_on = false;
static bool switch_synced = false; // force a real read on first scan

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

    // --- Linux workspace management layers 0–4 ---
    [0] = LAYOUT(MO(1), MO(2), LGUI(LALT(KC_LEFT)), LGUI(LALT(KC_RIGHT))),
    [1] = LAYOUT(_______, MO(2), LGUI(LSFT(LALT(KC_LEFT))), LGUI(LSFT(LALT(KC_RIGHT)))),
    [2] = LAYOUT(MO(1), _______, LGUI(KC_TAB), LGUI(LSFT(KC_TAB))),
    [3] = LAYOUT(_______, _______, LGUI(KC_L), KC_SYSTEM_SLEEP),

  // --- Linux window-management layers (4–7) ---
    [4] = LAYOUT(MO(5), MO(6), LGUI(KC_LEFT), LGUI(KC_RIGHT)),
    [5] = LAYOUT(_______, MO(6), LGUI(KC_UP), LGUI(KC_DOWN)),
    [6] = LAYOUT(MO(5), _______, LGUI(LALT(KC_LEFT)), LGUI(LALT(KC_RIGHT))),
    [7] = LAYOUT(_______, _______, LGUI(KC_L), KC_SYSTEM_SLEEP),
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
    setPinInputHigh(LAYER_SWITCH_PIN);

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
    state = update_tri_layer_state(state, 5, 6, 7);

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
    
    // ------ OS_Switch: select layer set ------
    bool new_mode = !readPin(LAYER_SWITCH_PIN);
    if (!switch_synced || new_mode != switch_on) {
        switch_synced       = true;
        switch_on           = new_mode;
        uint8_t target_base = switch_on ? BASE_LAYER_2 : BASE_LAYER_1;
        layer_move(target_base); // activate the correct base layer
    }

    // ------ GUI hold timeout ------
    if (gui_held && timer_elapsed32(last_tab_time) > GUI_HOLD_TIMEOUT) {
        unregister_mods(MOD_BIT(KC_LGUI));
        gui_held = false;
    }

    // ------ read slider --------
    if (timer_elapsed32(last_slider_read) < 20){
        return ;
    }
    last_slider_read = timer_read32();

    int16_t raw = analogReadPin(SLIDER_PIN);
    if (slider_anchor < 0){
        slider_anchor = raw; // adopt current position at boot, no tap
        return ;
    }

    while (raw - slider_anchor >= SLIDER_STEP){
        tap_code(KC_AUDIO_VOL_UP);
        slider_anchor += SLIDER_STEP;
    }
    while (slider_anchor - raw >= SLIDER_STEP){
        tap_code(KC_AUDIO_VOL_DOWN);
        slider_anchor -= SLIDER_STEP;
    }
}

// ----------------------
// Called on every keyevent, just handling keypresses
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // nothing pressed
    if (!record->event.pressed)
        return true;

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
