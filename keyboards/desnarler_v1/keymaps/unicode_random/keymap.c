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
// Layer definitions
// ----------------------
#define BASE_LAYER_1 0
#define BASE_LAYER_2 4

// custom keys
enum custom_keycodes {
    K0 = SAFE_RANGE,
    K1,
    K2,
    K3,
    K4,
    K5,
    K6,
    K7
};

// ----------------------
// Keymap
// ----------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // --- switch to left
    //[0] = LAYOUT(K0, K1, K2, K3),
    [0] = LAYOUT(KC_A, KC_B, KC_C, KC_D),

    // --- switch to right
    [4] = LAYOUT(K4, K5, K6, K7)

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

    writePinHigh(LED2_PIN);
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

    // ------ start slider once ------
    if (!slider_ready){
        last_vol_change = timer_read32();
        slider_ready = true;
        return ;
    }

    // ------ read slider --------
    int16_t raw = analogReadPin(SLIDER_PIN);
    if (timer_elapsed(last_vol_change) < 100){
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

// ----------------------
// Called on every keypress
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // nothing to register -> just return
    if (!record->event.pressed)
        return true;

    // Umlauts
    switch (keycode) {
        case K0: SEND_STRING("fish"); return false;
        case K1: SEND_STRING("pufferfish"); return false;
        case K2: SEND_STRING("turtle"); return false;
        case K3: SEND_STRING("egg"); return false;
        case K4: SEND_STRING("ä"); return false;
        case K5: SEND_STRING("broken_heart"); return false;
        case K6: SEND_STRING("two_hearts"); return false;
        case K7: SEND_STRING("sparkling_heart"); return false;
    }
    return true;
}
