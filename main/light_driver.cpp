/*
    Light driver
*/

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <esp_matter.h>
#include <common_macros.h>
#include <app_priv.h>
#include "led_driver.h"

using namespace chip::app::Clusters;
using namespace esp_matter;

static bool currentPowerState = false;
static uint8_t currentBrighness;
static uint16_t currentColorTemperature;
static const char *TAG = "light_driver";

// Set current brightness & color temperature
static void led_driver_set_current() {
    if (!currentPowerState) {
        return;
    }
    led_driver_set_pwm(currentBrighness, currentColorTemperature);
}

static void app_driver_light_set_power(bool power)
{
    ESP_LOGI(TAG, "LED set power: %d", power);
    if (power) {
        // Power on
        // led_driver_set_pwm(0, currentColorTemperature);
    } else {
        // Power off

        currentBrighness = 0;
        led_driver_set_current();
    }
    currentPowerState = power;
}

static void app_driver_light_set_brightness(uint8_t brightness)
{
    // int value = REMAP_TO_RANGE(brightness, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
    ESP_LOGI(TAG, "LED set brightness: %d", brightness);
    currentBrighness = brightness;
    led_driver_set_current();
}

static void app_driver_light_set_temperature(uint16_t mireds)
{
    uint32_t kelvin = REMAP_TO_RANGE_INVERSE(mireds, STANDARD_TEMPERATURE_FACTOR);
    ESP_LOGI(TAG, "LED set temperature: %ldK, %u", kelvin, mireds);
    currentColorTemperature = mireds;
    led_driver_set_current();
}

void app_driver_attribute_update(uint32_t cluster_id,
                                 uint32_t attribute_id,
                                 esp_matter_attr_val_t *val)
{
    switch (cluster_id) {
    case OnOff::Id:
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            app_driver_light_set_power(val->val.b);
        }
        break;
    case LevelControl::Id:
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            app_driver_light_set_brightness(val->val.u8);
        }
        break;
    case ColorControl::Id:
        if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            app_driver_light_set_temperature(val->val.u16);
        }
        break;
    }
}

#if CONFIG_NIGHT_LED_CLUSTER

void app_driver_attribute_update_night(uint32_t cluster_id,
                                       uint32_t attribute_id,
                                       esp_matter_attr_val_t *val)
{
    switch (cluster_id) {
    case OnOff::Id:
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            led_driver_set_night_led(val->val.b);
        }
        break;
    }
}

#endif

void app_driver_light_set_defaults(uint16_t endpoint_id)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t *attribute;

    /* Setting color */
    attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorMode::Id);
    attribute::get_val(attribute, &val);
    switch (val.val.u8)
    {
    case (uint8_t)ColorControl::ColorMode::kColorTemperature:
    {
        /* Setting temperature */
        ESP_LOGI(TAG, "LED set default temperature");
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTempPhysicalMaxMireds::Id);
        attribute::get_val(attribute, &val);
        auto miredsWarm = val.val.u16;
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTempPhysicalMinMireds::Id);
        attribute::get_val(attribute, &val);
        auto miredsCold = val.val.u16;
        led_driver_set_mireds_bounds(miredsWarm, miredsCold);

        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::get_val(attribute, &val);
        app_driver_light_set_temperature(val.val.u16);
        break;
    }
    default:
        ESP_LOGE(TAG, "Color mode not supported");
        break;
    }

    /* Setting power */
    attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    app_driver_light_set_power(val.val.b);

    /* Setting brightness */
    attribute = attribute::get(endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::get_val(attribute, &val);
    app_driver_light_set_brightness(val.val.u8);
}

#if CONFIG_NIGHT_LED_CLUSTER
void app_driver_night_led_set_defaults(uint16_t endpoint_id)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t *attribute;

    /* Setting power */
    attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    led_driver_set_night_led(val.val.b);
}
#endif

void app_driver_light_init() {
    led_driver_init();
}
