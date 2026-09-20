// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>

// Drives the single WS2812 NeoPixel built into the RP2040-Zero module
// (GP16), independent of the external rgblight strip on GP29.
void onboard_led_init(void);
void onboard_led_set(uint8_t red, uint8_t green, uint8_t blue);
