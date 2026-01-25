//
// XIAO ESP32C6 module
//

#include <esp_log.h>
#include <stdlib.h>
#include <app_priv.h>
#include "driver/gpio.h"

#ifdef CONFIG_XIAO_ESP32C6_EXTERNAL_ANTENNA

#define RF_SWITCH_ENABLE GPIO_NUM_3
#define WIFI_ANT_CONFIG GPIO_NUM_14

// Enable external XIAO antenna
void xiao_wifi_init()
{
    // enable the RF switch
    gpio_set_direction(RF_SWITCH_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(RF_SWITCH_ENABLE, 0);
    // select the external antenna
    gpio_set_direction(WIFI_ANT_CONFIG, GPIO_MODE_OUTPUT);
    gpio_set_level(WIFI_ANT_CONFIG, 1);
}

#endif
