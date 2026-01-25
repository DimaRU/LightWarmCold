//
// Indicator driver
//

#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include "indicator_driver.h"
#include "driver/ledc.h"
#include "soc/ledc_reg.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LEDC_FREQ_HZ    5000
#define LEDC_RES        LEDC_TIMER_12_BIT  // PWM resolution (13-bit = 0-4095 duty values)
#define MAX_DUTY        4095

enum class IndicateType : int {
    off,
    on,
    blink,      // blink cycle time
    breathe     // breathe cycle time
};

static void indicatorTask( void *pvParameters );

static const char *TAG = "indicator_driver";
static QueueHandle_t indicatorEventQueue;

static ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,      // timer mode
    .duty_resolution = LEDC_RES,            // resolution of PWM duty
    .timer_num = LEDC_TIMER_1,              // timer index
    .freq_hz = LEDC_FREQ_HZ,                // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK,               // Auto select the source clock
};

static ledc_channel_config_t ledcChannel = 
    {
        .gpio_num   = CONFIG_INDICATOR_LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_2,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,
        .hpoint     = 0,
#if CONFIG_INDICATOR_LED_INVERT == 1
        .flags = { .output_invert = 1 },
#endif
    };

void indicatorTask( void *pvParameters ) {
    int params[2];
    int cycleTime;

    ESP_LOGI(TAG, "Start indicator task");
    xQueueReceive(indicatorEventQueue, &params, portMAX_DELAY);
    for (;;)
    {
        switch (IndicateType(params[0]))
        {
        case IndicateType::off:
            ESP_LOGI(TAG, "indicator off");
            ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, 0, 0);
            // block
            xQueueReceive(indicatorEventQueue, &params, portMAX_DELAY);
            break;
        case IndicateType::on:
            ESP_LOGI(TAG, "indicator on");
            ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, MAX_DUTY, 0);
            // block
            xQueueReceive(indicatorEventQueue, &params, portMAX_DELAY);
            break;
        case IndicateType::blink: // blink cycle time
            cycleTime = params[1];
            ESP_LOGI(TAG, "indicator blink %d", cycleTime);
            do
            {
                ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, MAX_DUTY, 0);
                vTaskDelay(pdMS_TO_TICKS(cycleTime / 2));
                ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(cycleTime / 2));
            } while (!xQueueReceive(indicatorEventQueue, &params, 0));
            break;
        case IndicateType::breathe: // breathe cycle time
            cycleTime = params[1];
            ESP_LOGI(TAG, "indicator breathe %d", cycleTime);
            do
            {
                ledc_set_fade_with_time(ledcChannel.speed_mode, ledcChannel.channel, MAX_DUTY, cycleTime / 2);
                ledc_fade_start(ledcChannel.speed_mode, ledcChannel.channel, LEDC_FADE_WAIT_DONE);
                ledc_set_fade_with_time(ledcChannel.speed_mode, ledcChannel.channel, 0, cycleTime / 2);
                ledc_fade_start(ledcChannel.speed_mode, ledcChannel.channel, LEDC_FADE_WAIT_DONE);
            } while (!xQueueReceive(indicatorEventQueue, &params, 0));
            break;
        }
    }
}


void signalIndicator(enum SignalIndicator signal) {
    static bool comissioningInProgress = false;
    int params[2];

    switch (signal)
    {
    case SignalIndicator::startup: // Powered on
        params[0] = static_cast<int>(IndicateType::on);
        break;
    case SignalIndicator::connected: // Thread/Wifi connection established
        params[0] = static_cast<int>(IndicateType::off);
        break;
    case SignalIndicator::commissioningOpen: // Commissioning window opened
        params[0] = static_cast<int>(IndicateType::breathe);
        params[1] = 1500; // breathe cycle in ms
        break;
    case SignalIndicator::commissioningStart: // Commissioning session started
        comissioningInProgress = true;
        params[0] = static_cast<int>(IndicateType::breathe);
        params[1] = 700; // breathe cycle in ms
        break;
    case SignalIndicator::commissioningStop: // Commissioning complete/failed
        comissioningInProgress = false;
        params[0] = static_cast<int>(IndicateType::off);
        break;
    case SignalIndicator::commissioningClose: // Commissioning window closed
        if (comissioningInProgress) {
            return;
        }
        params[0] = static_cast<int>(IndicateType::off);
        break;
    case SignalIndicator::identificationStart:
        params[0] = static_cast<int>(IndicateType::blink);
        params[1] = 250; // blink cycle in ms
        break;
    case SignalIndicator::identificationStop:
        params[0] = static_cast<int>(IndicateType::off);
        break;
    }
    xQueueSend(indicatorEventQueue, params, 0);
}

void indicator_driver_init()
{
    ESP_LOGI(TAG, "indicator driver init");
    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledcChannel);
    // ledc_fade_func_install(0);
    indicatorEventQueue = xQueueCreate(10, sizeof(int)*2);
    xTaskCreate(indicatorTask, "indicatorTask", 2048, nullptr, 14, nullptr);
}
