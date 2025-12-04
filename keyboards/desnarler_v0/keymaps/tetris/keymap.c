#include "analog.h"
#include QMK_KEYBOARD_H

// ----------------------
// Slider setup
// ----------------------
#define SLIDER_PIN 26
#define MAX_SLIDER_STEPS 100
#define SLIDER_DEADBAND 2 // ignore <2 steps of change
//static int16_t last_val = 0;

// ----------------------
// Button setup (GP3 as Enter)
// ----------------------
#define ENTER_PIN GP3
static bool enter_pressed = false;

// ----------------------
// LEDs
// ----------------------
#define LED1_PIN 29 // left LED
#define LED2_PIN 27
#define LED3_PIN 28 // right LED

// ----------------------
// helpers to hold tab
// ----------------------
static bool     gui_held      = false;
static uint32_t last_tab_time = 0;
#define GUI_HOLD_TIMEOUT 1000 // ms

// ----------------------
// Layer definitions
// ----------------------
#define BASE_LAYER 0

// ----------------------
// Keymap
// ----------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[BASE_LAYER] = LAYOUT(KC_ENTER, KC_LEFT, KC_RIGHT, KC_X)};

// ----------------------
// Called once at boot
// ----------------------
void matrix_init_user(void) {
    // Initialize Enter button (GP3)
    setPinInputHigh(ENTER_PIN); // enable pull-up

    // Initialize LEDs
    setPinOutput(LED1_PIN);
    setPinOutput(LED2_PIN);
    setPinOutput(LED3_PIN);

    // Initial LED animation
    for (int i = 0; i < 6; i++) {
        writePinHigh(LED1_PIN);
        writePinLow(LED2_PIN);
        writePinHigh(LED3_PIN);
        wait_ms(150);
        writePinLow(LED1_PIN);
        writePinHigh(LED2_PIN);
        writePinLow(LED3_PIN);
        wait_ms(150);
    }

    writePinLow(LED1_PIN);
    writePinLow(LED2_PIN);
    writePinLow(LED3_PIN);
}

// ----------------------
// Called when layer changes
// ----------------------
layer_state_t layer_state_set_user(layer_state_t state) {
    // always single layer, just LED feedback
    writePinHigh(LED1_PIN);
    writePinLow(LED2_PIN);
    writePinLow(LED3_PIN);
    return state;
}

// ----------------------
// matrix_scan_user: repeated loop
// ----------------------
void matrix_scan_user(void) {
    // ------ Check Enter button (GP3) ------
    bool pressed = !readPin(ENTER_PIN); // pulled-up → active low
    if (pressed && !enter_pressed) {
        register_code(KC_ENTER);
        enter_pressed = true;
    } else if (!pressed && enter_pressed) {
        unregister_code(KC_ENTER);
        enter_pressed = false;
    }

    // ------ GUI hold timeout ------
    if (gui_held && timer_elapsed32(last_tab_time) > GUI_HOLD_TIMEOUT) {
        unregister_mods(MOD_BIT(KC_LGUI));
        gui_held = false;
    }
/*
    // ------ Slider processing ------
    int16_t raw    = analogReadPin(SLIDER_PIN);
    int     target = (int)(raw * MAX_SLIDER_STEPS / 4095.0f);
    if (target < 0) target = 0;
    if (target > MAX_SLIDER_STEPS) target = MAX_SLIDER_STEPS;

    if (abs(target - last_val) <= SLIDER_DEADBAND) return; // ignore noise

    if (target > last_val) {
        // Slider moved up → Zoom In (Ctrl + '+')
        register_code(KC_LCTL);
        tap_code(KC_EQUAL);
        unregister_code(KC_LCTL);
    } else {
        // Slider moved down → Zoom Out (Ctrl + '-')
        register_code(KC_LCTL);
        tap_code(KC_MINUS);
        unregister_code(KC_LCTL);
    }

    last_val = target;
*/
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

    // Handle GUI tab holding
    switch (keycode) {
        case LGUI(KC_TAB):
        case LGUI(LSFT(KC_TAB)):
            if (!gui_held) {
                register_mods(MOD_BIT(KC_LGUI));
                gui_held = true;
            }

            if (keycode == LGUI(KC_TAB))
                tap_code(KC_TAB);
            else {
                register_mods(MOD_BIT(KC_LSFT));
                tap_code(KC_TAB);
                unregister_mods(MOD_BIT(KC_LSFT));
            }
            last_tab_time = timer_read32();
            return false;
    }

    return true;
}
