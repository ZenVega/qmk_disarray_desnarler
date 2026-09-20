// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Minimal, blocking WS2812 driver for the single onboard NeoPixel on the
// Waveshare RP2040-Zero (GP16). Runs on PIO1 so it doesn't interfere with
// the external rgblight strip, which already owns PIO0 via WS2812_DI_PIN.

#include "onboard_led.h"

// Keep this exact include order otherwise we run into naming conflicts
// between pico-sdk and rp2040.h which we don't control.
#include "hardware/clocks.h"
#include <hal.h>
#include "hardware/pio.h"

#include "gpio.h"
#include "util.h"

#ifndef ONBOARD_LED_PIN
#    define ONBOARD_LED_PIN GP16
#endif

#define ONBOARD_LED_PIO pio1

// Standard WS2812 timings (ns), same defaults as drivers/led/ws2812.h.
#define OL_TIMING 1250
#define OL_T1H 900
#define OL_T1L (OL_TIMING - OL_T1H)
#define OL_T0H 350
#define OL_T0L (OL_TIMING - OL_T0H)

#define OL_PIO_T1L (OL_T1L / 50)
#define OL_PIO_T1L_A (MAX(CEILING(OL_PIO_T1L, 2) - 1, 0))
#define OL_PIO_T1L_B (MAX(OL_PIO_T1L / 2 - 1, 0))
#define OL_PIO_T0L (MAX(OL_T0L / 50 - OL_PIO_T1L, 0))
#define OL_PIO_T0L_A (MAX(OL_PIO_T0L - 1, 0))
#define OL_PIO_T0H (OL_T0H / 50)
#define OL_PIO_T0H_A MAX(OL_PIO_T0H - 1, 0)
#define OL_PIO_T1H (MAX(OL_T1H / 50 - OL_PIO_T0H, 0))
#define OL_PIO_T1H_A (MAX((CEILING(OL_PIO_T1H, 2) - 1), 0))
#define OL_PIO_T1H_B (MAX((OL_PIO_T1H / 2) - 1, 0))

#define OL_PIO_DELAY(delay, opcode) (((delay & 0xF) << 8U) | opcode)

#define OL_WRAP_TARGET 0
#define OL_WRAP 5

static const uint16_t onboard_led_program_instructions[] = {
    //     .wrap_target
    OL_PIO_DELAY(OL_PIO_T1L_A, 0x6021), //  0: out    x, 1            side 0  // T1L
    OL_PIO_DELAY(OL_PIO_T1L_B, 0xa042), //  1: nop                    side 0  // T1L
    OL_PIO_DELAY(OL_PIO_T0H_A, 0x1025), //  2: jmp    !x, 5           side 1  // T0H
    OL_PIO_DELAY(OL_PIO_T1H_A, 0xb042), //  3: nop                    side 1  // T1H
    OL_PIO_DELAY(OL_PIO_T1H_B, 0x1000), //  4: jmp    0               side 1  // T1H
    OL_PIO_DELAY(OL_PIO_T0L_A, 0xa042), //  5: nop                    side 0  // T0L
    //     .wrap
};

static const pio_program_t onboard_led_program = {
    .instructions = onboard_led_program_instructions,
    .length       = ARRAY_SIZE(onboard_led_program_instructions),
    .origin       = -1,
};

static const PIO pio          = ONBOARD_LED_PIO;
static int       state_machine = -1;

void onboard_led_init(void) {
    uint pio_idx = pio_get_index(pio);
    hal_lld_peripheral_unreset(pio_idx == 0 ? RESETS_ALLREG_PIO0 : RESETS_ALLREG_PIO1);

    palSetLineMode(ONBOARD_LED_PIN, PAL_RP_PAD_SLEWFAST | PAL_RP_GPIO_OE | (pio_idx == 0 ? PAL_MODE_ALTERNATE_PIO0 : PAL_MODE_ALTERNATE_PIO1));

    state_machine = pio_claim_unused_sm(pio, true);
    if (state_machine < 0) {
        return;
    }

    uint offset = pio_add_program(pio, &onboard_led_program);

    pio_sm_set_consecutive_pindirs(pio, state_machine, ONBOARD_LED_PIN, 1, true);

    pio_sm_config config = pio_get_default_sm_config();
    sm_config_set_wrap(&config, offset + OL_WRAP_TARGET, offset + OL_WRAP);
    sm_config_set_sideset_pins(&config, ONBOARD_LED_PIN);
    sm_config_set_sideset(&config, 1, false, false);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
    sm_config_set_out_shift(&config, false, true, 24);

    // Every instruction takes 50ns to execute with a clock speed of 20 MHz.
    float div = clock_get_hz(clk_sys) / (20.0f * MHZ);
    sm_config_set_clkdiv(&config, div);

    pio_sm_init(pio, state_machine, offset, &config);
    pio_sm_set_enabled(pio, state_machine, true);
}

void onboard_led_set(uint8_t red, uint8_t green, uint8_t blue) {
    if (state_machine < 0) {
        return;
    }
    uint32_t grb = ((uint32_t)green << 24) | ((uint32_t)red << 16) | ((uint32_t)blue << 8);
    pio_sm_put_blocking(pio, state_machine, grb);
}
