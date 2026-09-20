// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

// ---------------------------------------------------------------------------
// macro_goat_v0 — default keymap
//
// This file is meant as a starting point: copy this folder to
// keyboards/macro_goat_v0/keymaps/<your_name>/ and edit away, then flash with
//
//     qmk compile -kb macro_goat_v0 -km <your_name>
//     qmk flash   -kb macro_goat_v0 -km <your_name>
//
// The hardware you are mapping:
//   * 8 keys, wired as a 2 x 4 matrix
//   * 4 rotary encoders
//   * 16 WS2812 LEDs on GP29 ("rgblight", the under/backlight strip)
//   * 1 WS2812 LED built into the RP2040-Zero board itself (the "power LED",
//     driven separately by onboard_led.c)
//
// Sections below, in order:
//   1. Tunables            — timings and brightness for the power LED
//   2. Keymap              — what each key sends            <- edit this
//   3. Encoder map         — what each encoder sends        <- edit this
//   4. Encoder side effect — extra code run on every twist  <- edit this
//   5. Lighting behaviour  — startup animation + hue cycling
// ---------------------------------------------------------------------------

#include QMK_KEYBOARD_H
#include "onboard_led.h"

// --- 1. Tunables -----------------------------------------------------------

// How long the rainbow swirl plays after plugging in, before the strip settles
// into a single static colour. Set to 0 for no startup animation.
#define STARTUP_MS 3000

// The onboard power LED "breathes" blue. Bigger STEP or smaller STEP_MS makes
// it pulse faster; MAX caps how bright it gets (0-255).
#define POWER_LED_STEP_MS 20
#define POWER_LED_STEP 4
#define POWER_LED_MAX 180

// --- 2. Keymap -------------------------------------------------------------
//
// Physical layout — matrix positions map to LAYOUT() arguments in this order:
//
//     ┌───┬───┬───┬───┐
//     │ 1 │ 2 │ 3 │ 4 │   top row    (matrix row 0)
//     ├───┼───┼───┼───┤
//     │ 5 │ 6 │ 7 │ 8 │   bottom row (matrix row 1)
//     └───┴───┴───┴───┘
//
// Replace the KC_* keycodes below with whatever you want. A few ideas:
//   KC_F13 .. KC_F24        keys no application uses — perfect for macro
//                           bindings in OBS, Blender, your window manager, ...
//   LCTL(KC_C)              Ctrl+C (and LSFT/LALT/LGUI for the other mods)
//   LCTL(LSFT(KC_ESC))      stacked modifiers
//   MO(1)                   hold for layer 1 (see below about extra layers)
//   TO(1), TG(1)            switch to / toggle layer 1
//   KC_MPLY, KC_MNXT        media transport
//   QK_BOOT                 reboot into the bootloader for flashing
//
// Adding a second layer: add another row to this array and to encoder_map,
// and give yourself a way to reach it, e.g.
//
//     const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
//         [0] = LAYOUT(KC_1,   KC_2, KC_3, KC_4,
//                      MO(1),  KC_6, KC_7, KC_8),
//         [1] = LAYOUT(KC_F13, KC_F14, KC_F15, KC_F16,
//                      _______, KC_F18, KC_F19, KC_F20),
//     };
//
// (_______ = "fall through to the layer below", KC_NO = "do nothing".)
// config.h reserves 8 layers for VIA; plain firmware layers are limited only
// by flash space.

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    [0] = LAYOUT(
        KC_1, KC_2, KC_3, KC_4,
        KC_5, KC_6, KC_7, KC_8
    ),
    // clang-format on
};

// --- 3. Encoder map --------------------------------------------------------
//
// One ENCODER_CCW_CW(counter_clockwise, clockwise) entry per encoder, in the
// order the encoders are declared in keyboard.json (GP6/7, GP8/9, GP10/11,
// GP12/13). Each keymap layer needs its own row here.
//
// Right now all four encoders do the same thing: volume. Handy alternatives:
//   ENCODER_CCW_CW(KC_MS_WH_UP, KC_MS_WH_DOWN)   scroll wheel
//   ENCODER_CCW_CW(KC_MPRV, KC_MNXT)             track skip
//   ENCODER_CCW_CW(LCTL(KC_MINS), LCTL(KC_PLUS)) zoom out / in
//   ENCODER_CCW_CW(KC_LEFT, KC_RGHT)             arrow keys / timeline scrub
//
// Note: volume up on counter-clockwise is intentional here — swap the two
// keycodes if you prefer the opposite direction.

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    // clang-format off
    [0] = {
        ENCODER_CCW_CW(KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN),
        ENCODER_CCW_CW(KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN),
        ENCODER_CCW_CW(KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN),
        ENCODER_CCW_CW(KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN),
    },
    // clang-format on
};
#endif

// --- 4. Encoder side effect ------------------------------------------------
//
// Runs on every encoder detent, *in addition to* the encoder_map above:
// returning true tells QMK to also send the mapped keycode. Return false here
// instead if you want an encoder to only run custom code and send nothing.
//
// `index` is the encoder number (0-3), so you can give each one its own
// behaviour:
//
//     if (index == 0) { ... } else if (index == 1) { ... }
//
// As shipped, turning any encoder also dims (clockwise) or brightens
// (counter-clockwise) the LED strip.

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (clockwise) {
        rgblight_decrease_val_noeeprom();
    } else {
        rgblight_increase_val_noeeprom();
    }
    return true; // false = swallow the encoder_map keycode
}

// --- 5. Lighting behaviour -------------------------------------------------
//
// Everything below drives the LEDs. It is self-contained: delete it (and the
// `#include "onboard_led.h"` plus the `SRC += onboard_led.c` line in rules.mk)
// if you just want a plain macropad.
//
// Two independent lights:
//   * the 16-LED strip on GP29, handled by QMK's rgblight_* API
//   * the single LED on the RP2040-Zero board, handled by onboard_led_set()
//
// Behaviour: the strip plays a rainbow swirl for STARTUP_MS, then locks to a
// static colour that advances one hue step on every keypress. The onboard LED
// continuously breathes blue.

static uint32_t startup_timer = 0;
static bool     startup_done  = false;
static uint8_t  led_hue       = 0;

static uint32_t power_led_timer = 0;
static int16_t  power_led_val   = 0;
static int16_t  power_led_dir   = POWER_LED_STEP;

// Non-blocking triangle-wave fade for the onboard LED. Change the argument
// order in onboard_led_set() to breathe in a different colour — it takes
// plain (red, green, blue) values, so e.g. (power_led_val, 0, 0) is red.
static void power_led_task(void) {
    if (timer_elapsed32(power_led_timer) < POWER_LED_STEP_MS) {
        return;
    }
    power_led_timer = timer_read32();

    power_led_val += power_led_dir;
    if (power_led_val >= POWER_LED_MAX) {
        power_led_val = POWER_LED_MAX;
        power_led_dir = -POWER_LED_STEP;
    } else if (power_led_val <= 0) {
        power_led_val = 0;
        power_led_dir = POWER_LED_STEP;
    }

    onboard_led_set(0, 0, (uint8_t)power_led_val);
}

// Re-apply the current hue at full saturation, keeping whatever brightness the
// encoders have dialled in. _noeeprom keeps the flash from being worn out by
// every keypress — the cost is that colours reset on unplug.
static void refresh_leds(void) {
    rgblight_sethsv_noeeprom(led_hue, 255, rgblight_get_val());
}

// Called once after the keyboard finishes booting.
void keyboard_post_init_user(void) {
    onboard_led_init();
    rgblight_enable_noeeprom();
    rgblight_sethsv_noeeprom(led_hue, 255, 60); // 60 = starting brightness
    rgblight_mode_noeeprom(RGBLIGHT_MODE_RAINBOW_SWIRL);
    startup_timer = timer_read32();
}

// Called continuously, thousands of times a second — never block in here
// (no wait_ms()), always use timer_elapsed32() as the tasks above do.
void matrix_scan_user(void) {
    power_led_task();

    // End of the startup animation: freeze the strip on a single colour.
    if (!startup_done && timer_elapsed32(startup_timer) >= STARTUP_MS) {
        rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
        startup_done = true;
        refresh_leds();
    }
}

// Called for every key event before QMK handles it. Returning true lets the
// keycode through; return false to consume it, which is how you implement
// custom keycodes and macros:
//
//     switch (keycode) {
//         case KC_1:
//             if (record->event.pressed) SEND_STRING("hello world");
//             return false;
//     }
//
// Here it just advances the strip's hue by RGBLIGHT_HUE_STEP (set in config.h)
// on each press, once the startup animation is over.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed && startup_done) {
        led_hue += RGBLIGHT_HUE_STEP;
        refresh_leds();
    }
    return true;
}
