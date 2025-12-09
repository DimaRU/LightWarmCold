//
// Two color led driver
//

#pragma once

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

void led_driver_init();
void led_driver_set_power(bool power);
void led_driver_set_pwm(uint8_t brightness, int16_t temperature);
void setMiredsBounds(uint16_t warm, uint16_t cool);
