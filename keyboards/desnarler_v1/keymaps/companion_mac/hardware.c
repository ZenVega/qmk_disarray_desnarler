// Hardware callbacks: LEDs, toggle switch, slider.
// Compiled as a regular source file — has full QMK platform headers.
#include QMK_KEYBOARD_H
#include "analog.h"
#include "gpio.h"

#define SLIDER_PIN 29

static uint32_t last_vol_change = 0;
static bool     switch_on       = false;
static int32_t  slider_smooth   = -1;  // exponential moving average (-1 = uninitialized)
static int16_t  slider_ref      = -1;  // reference point; only resets when we fire

#define SLIDER_DEAD_ZONE 80  // increase if still too sensitive

void matrix_init_user(void) {
    gpio_set_pin_input_high(LAYER_SWITCH_PIN);
    gpio_set_pin_output(LED1_PIN);
    gpio_set_pin_output(LED2_PIN);
    gpio_set_pin_output(LED3_PIN);

    for (int i = 0; i < 5; i++) {
        gpio_write_pin_high(LED1_PIN); gpio_write_pin_low(LED2_PIN);  gpio_write_pin_high(LED3_PIN); wait_ms(150);
        gpio_write_pin_low(LED1_PIN);  gpio_write_pin_high(LED2_PIN); gpio_write_pin_low(LED3_PIN);  wait_ms(150);
    }
    gpio_write_pin_low(LED2_PIN);
    gpio_write_pin_high(LED1_PIN);
}

layer_state_t layer_state_set_user(layer_state_t state) {
    gpio_write_pin_low(LED1_PIN);
    gpio_write_pin_low(LED2_PIN);
    gpio_write_pin_low(LED3_PIN);
    if (get_highest_layer(state) == 0) {
        gpio_write_pin_high(LED1_PIN);
    } else {
        gpio_write_pin_high(LED3_PIN);
    }
    return state;
}

void matrix_scan_user(void) {
    bool new_mode = !gpio_read_pin(LAYER_SWITCH_PIN);
    if (new_mode != switch_on) {
        switch_on = new_mode;
        layer_move(switch_on ? 1 : 0);
    }

    if (timer_elapsed(last_vol_change) < 60) return;
    last_vol_change = timer_read32();

    int16_t raw = analogReadPin(SLIDER_PIN);

    // Exponential moving average (7/8 old + 1/8 new) to kill ADC noise.
    if (slider_smooth == -1) {
        slider_smooth = raw;
        slider_ref    = raw;
        return;
    }
    slider_smooth = (slider_smooth * 7 + raw) / 8;

    // Only fire when smoothed value has moved significantly from the last
    // reference point. Reference only resets after a trigger — not every tick —
    // so ADC noise cannot accumulate into a false event.
    int16_t delta = (int16_t)slider_smooth - slider_ref;
    if (delta < -SLIDER_DEAD_ZONE) {
        tap_code(KC_AUDIO_VOL_DOWN);
        slider_ref = (int16_t)slider_smooth;
    } else if (delta > SLIDER_DEAD_ZONE) {
        tap_code(KC_AUDIO_VOL_UP);
        slider_ref = (int16_t)slider_smooth;
    }
}
