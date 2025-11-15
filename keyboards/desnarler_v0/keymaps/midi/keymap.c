#include "analog.h"
#include QMK_KEYBOARD_H
// ---------------------
// Hardware definitions
// ---------------------
#define SLIDER_PIN 26  // works on your hardware
#define SWITCH_PIN GP3 // mode switch pin
// ---------------------
// MIDI setup
// ---------------------
extern MidiDevice midi_device;    // << required for classic QMK MIDI
static uint8_t    base_note = 60; // middle C
static bool       midi_mode = false;
// ---------------------
// QMK setup
// ---------------------
void matrix_init_user(void) {
    setPinInputHigh(SWITCH_PIN); // enable pull-up on mode switch
}
// ---------------------
// Main loop
// ---------------------
void matrix_scan_user(void) {
    midi_mode = !readPin(SWITCH_PIN); // active-low toggle
    if (midi_mode) {
        uint16_t raw = analogReadPin(SLIDER_PIN);

        // Your slider's actual range (from earlier testing)
        const uint16_t min_val = 24;
        const uint16_t max_val = 993;

        // Clamp to range
        if (raw < min_val) raw = min_val;
        if (raw > max_val) raw = max_val;

        // Remap to full 0-4095 range for calculations
        uint16_t scaled = ((raw - min_val) * 4095UL) / (max_val - min_val);

        // Map to wide note range: C1 (24) to C8 (108) = 84 semitones (7 octaves)
        base_note = 24 + (scaled * 84 / 4095);
    }
}
// ---------------------
// Key actions
// ---------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!midi_mode) return true; // Pass through when MIDI mode off

    switch (keycode) {
        case KC_1: // Single note
            if (record->event.pressed)
                midi_send_noteon(&midi_device, 0, base_note, 127);
            else
                midi_send_noteoff(&midi_device, 0, base_note, 0);
            return false; // Block keycode

        case KC_2: // Major chord
            if (record->event.pressed) {
                midi_send_noteon(&midi_device, 0, base_note, 127);
                midi_send_noteon(&midi_device, 0, base_note + 4, 127);
                midi_send_noteon(&midi_device, 0, base_note + 7, 127);
            } else {
                midi_send_noteoff(&midi_device, 0, base_note, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 4, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 7, 0);
            }
            return false; // Block keycode

        case KC_3: // Minor chord
            if (record->event.pressed) {
                midi_send_noteon(&midi_device, 0, base_note, 127);
                midi_send_noteon(&midi_device, 0, base_note + 3, 127);
                midi_send_noteon(&midi_device, 0, base_note + 7, 127);
            } else {
                midi_send_noteoff(&midi_device, 0, base_note, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 3, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 7, 0);
            }
            return false; // Block keycode

        case KC_4: // Dominant 7th chord
            if (record->event.pressed) {
                midi_send_noteon(&midi_device, 0, base_note, 127);
                midi_send_noteon(&midi_device, 0, base_note + 4, 127);
                midi_send_noteon(&midi_device, 0, base_note + 7, 127);
                midi_send_noteon(&midi_device, 0, base_note + 10, 127);
            } else {
                midi_send_noteoff(&midi_device, 0, base_note, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 4, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 7, 0);
                midi_send_noteoff(&midi_device, 0, base_note + 10, 0);
            }
            return false; // Block keycode
    }

    return true; // Pass through other keys
}
// ---------------------
// Basic layout
// ---------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT(KC_1, KC_2, KC_3, KC_4)};
