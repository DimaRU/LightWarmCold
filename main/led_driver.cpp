//
// Two color led driver
//

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <common_macros.h>
#include "app_priv.h"
#include "led_driver.h"
#include "driver/ledc.h"
#include "soc/ledc_reg.h"

static void fadeTask( void *pvParameters );
static void led_driver_queue_pwm(uint32_t warmPWM, uint32_t coldPWM);

static const char *TAG = "led_driver";

static uint16_t MiredsWarm;
static uint16_t MiredsCold;

static QueueHandle_t fadeEventQueue;

static ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,        // timer mode
    .duty_resolution = LEDC_TIMER_12_BIT,     // resolution of PWM duty
    .timer_num = LEDC_TIMER_0,                // timer index
    .freq_hz = CONFIG_PWM_FREQUENCY,          // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK,                 // Auto select the source clock
};

static ledc_channel_config_t ledcChannel[2] = {
    {
        .gpio_num   = CONFIG_LED_WARM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    },
    {
        .gpio_num   = CONFIG_LED_COLD_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    },
};

static uint32_t PWMBase = 1 << LEDC_TIMER_12_BIT;

static void fadeTask( void *pvParameters ) {
    uint32_t pwm[3];
    int fadeTime;

    ESP_LOGI(TAG, "Init fade task chan");
    for( ;; ) {
        if (xQueueReceive(fadeEventQueue, &pwm, portMAX_DELAY)) {
            fadeTime = pwm[2];
            for(int chan = 0; chan < 2; chan++) {
                // const int duty = CIEL_10_12[fade->target];
                int duty = pwm[chan];
                ledc_set_fade_with_time(ledcChannel[chan].speed_mode, ledcChannel[chan].channel, duty, fadeTime);
            }
            ledc_fade_start(ledcChannel[0].speed_mode, ledcChannel[0].channel, LEDC_FADE_NO_WAIT);
            ledc_fade_start(ledcChannel[1].speed_mode, ledcChannel[1].channel, LEDC_FADE_WAIT_DONE);
        }
    }
}

static void led_driver_queue_pwm(uint32_t warmPWM, uint32_t coldPWM) {
    static uint32_t currentPWM[2] = { 0, 0 };

    uint32_t pwm[3];
    pwm[0] = warmPWM;
    pwm[1] = coldPWM;

    uint32_t fadeTime = 0;
    for(int chan = 0; chan < 2; chan++) {
        uint32_t time = 0;
        if (currentPWM[chan] > pwm[chan]) {
            time = (currentPWM[chan] - pwm[chan]) * CONFIG_FADE_TIME / PWMBase;
        } else {
            time = (pwm[chan] - currentPWM[chan]) * CONFIG_FADE_TIME / PWMBase;
        }
        // const int duty = CIEL_10_12[fade->target];
        currentPWM[chan] = pwm[chan];
        if (time > fadeTime) {
            fadeTime = time;
        }
    }
    pwm[2] = fadeTime;

    ESP_LOGI(TAG, "pwm: %lu, warmCoeff: %lu, coldCoeff: %lu, time: %lu", PWMBase, warmPWM, coldPWM, fadeTime);
    
    xQueueSend(fadeEventQueue, pwm, 0);
}

// Public interface

void led_driver_set_pwm(uint8_t brightness, int16_t temperature) {
    uint32_t topMargin = 2;
    uint32_t miredsNeutral = (MiredsWarm + MiredsCold) / 2;
    
    uint32_t tempCoeff = (temperature - MiredsCold) * PWMBase / (MiredsWarm - MiredsCold);
    uint32_t brightnessCoeff = uint32_t(brightness) * PWMBase / uint32_t(MATTER_BRIGHTNESS);

    uint32_t warmPWM;
    uint32_t coldPWM;
    
    if (temperature >= miredsNeutral) {
        coldPWM = topMargin * (PWMBase - tempCoeff) * brightnessCoeff / PWMBase;
        warmPWM = brightnessCoeff;
    } else {
        warmPWM = topMargin * tempCoeff * brightnessCoeff / PWMBase;
        coldPWM = brightnessCoeff;
    }

    led_driver_queue_pwm(warmPWM, coldPWM);
}

#if CONFIG_NIGHT_LED_CLUSTER

void led_driver_set_night_led(bool on) {
    gpio_set_level(gpio_num_t(CONFIG_NIGHT_LED_GPIO), on);
}
#endif



void led_driver_init()
{
    ledc_timer_config(&ledc_timer);
    
    fadeEventQueue = xQueueCreate(10, sizeof(int32_t)*3);
    
    for(int chan = 0; chan < 2; chan++) {
        ledc_channel_config(&ledcChannel[chan]);
    }

    xTaskCreate(fadeTask, "fadeTask", 2048, nullptr, 15, nullptr);
    
    ledc_fade_func_install(0);

#if CONFIG_NIGHT_LED_CLUSTER
    // Set pin for output
    gpio_reset_pin(gpio_num_t(CONFIG_NIGHT_LED_GPIO));
    gpio_set_direction(gpio_num_t(CONFIG_NIGHT_LED_GPIO), GPIO_MODE_OUTPUT);

#endif
}

void led_driver_set_mireds_bounds(uint16_t warm, uint16_t cool)
{
    MiredsWarm = warm;
    MiredsCold = cool;
}
