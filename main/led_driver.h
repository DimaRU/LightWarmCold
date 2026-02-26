//
// Two color led driver
//

#pragma once

#include <stdlib.h>

void led_driver_init();
void led_driver_set_bounds(uint16_t warm, uint16_t cool, uint8_t minBrightness, uint8_t maxBrightness);
void led_driver_set_pwm(uint8_t brightness, int16_t temperature);
#if CONFIG_NIGHT_LED_CLUSTER
void led_driver_set_night_led(bool on);
#endif
