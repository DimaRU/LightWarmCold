/*
    Light driver
*/

#include <esp_log.h>
#include <stdlib.h>

#include <esp_matter.h>
#include "common_macros.h"
#include "app_priv.h"
#include "light_driver.h"
#include "led_driver.h"

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static bool currentPowerState = false;
static uint8_t currentBrightness;
static uint16_t currentColorTemperature;
static uint16_t light_endpoint_id;
#if CONFIG_NIGHT_LED_CLUSTER
static uint16_t night_light_endpoint_id;
#endif

static const char *TAG = "light_driver";

// Set current brightness & color temperature
static void led_driver_set_current() {
    if (!currentPowerState) {
        return;
    }
    led_driver_set_pwm(currentBrightness, currentColorTemperature);
}

static void app_driver_light_set_power(bool power)
{
    ESP_LOGI(TAG, "LED set power: %d", power);
    if (power) {
        // Power on
        // led_driver_set_pwm(currentBrightness, currentColorTemperature);
    } else {
        // Power off

        // currentBrightness = 0;
        led_driver_set_pwm(0, currentColorTemperature);
    }
    currentPowerState = power;
}

static void app_driver_light_set_brightness(uint8_t brightness)
{
    // int value = REMAP_TO_RANGE(brightness, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
    ESP_LOGI(TAG, "LED set brightness: %d", brightness);
    currentBrightness = brightness;

    led_driver_set_current();
}

static void app_driver_light_set_temperature(uint16_t mireds)
{
    uint32_t kelvin = REMAP_TO_RANGE_INVERSE(mireds, STANDARD_TEMPERATURE_FACTOR);
    ESP_LOGI(TAG, "LED set temperature: %ldK, %u", kelvin, mireds);
    currentColorTemperature = mireds;
    led_driver_set_current();
}

void app_driver_attribute_update(uint16_t endpoint_id,
                                 uint32_t cluster_id,
                                 uint32_t attribute_id,
                                 esp_matter_attr_val_t *val)
{
    if (endpoint_id == light_endpoint_id) {
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
        return;
    }

#if CONFIG_NIGHT_LED_CLUSTER
    if (endpoint_id == night_light_endpoint_id) {
        switch (cluster_id) {
        case OnOff::Id:
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                led_driver_set_night_led(val->val.b);
            }
            break;
        }
        return;
    }
#endif
}


static void app_driver_light_set_defaults(uint16_t endpoint_id)
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
        // Brightness bounds
        uint8_t minBrightness = 1;
        attribute = attribute::get(endpoint_id, LevelControl::Id, LevelControl::Attributes::MinLevel::Id);
        if (attribute != nullptr) {
            attribute::get_val(attribute, &val);
            minBrightness = val.val.u8;
        }

        uint8_t maxBrightness = MATTER_BRIGHTNESS;
        attribute = attribute::get(endpoint_id, LevelControl::Id, LevelControl::Attributes::MaxLevel::Id);
        if (attribute != nullptr) {
            attribute::get_val(attribute, &val);
            maxBrightness = val.val.u8;
        }
        // Temperature bounds
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTempPhysicalMaxMireds::Id);
        attribute::get_val(attribute, &val);
        auto miredsWarm = val.val.u16;
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTempPhysicalMinMireds::Id);
        attribute::get_val(attribute, &val);
        auto miredsCold = val.val.u16;
        
        led_driver_set_bounds(miredsWarm, miredsCold, minBrightness, maxBrightness);
        
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::get_val(attribute, &val);
        ESP_LOGI(TAG, "LED set default temperature");
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
static void app_driver_night_led_set_defaults(uint16_t endpoint_id)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t *attribute;

    /* Setting power */
    attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    led_driver_set_night_led(val.val.b);
}
#endif

void app_driver_create_endpoints(esp_matter::node_t *node) {
    color_temperature_light::config_t light_config;
    light_config.on_off.on_off = DEFAULT_POWER;
    light_config.on_off_lighting.start_up_on_off = nullptr;
    light_config.level_control.current_level = CONFIG_DEFAULT_BRIGHTNESS;
    light_config.level_control.on_level = nullptr;
    light_config.level_control_lighting.start_up_current_level = nullptr;
    light_config.level_control_lighting.min_level = 1;
    light_config.level_control_lighting.max_level = MATTER_BRIGHTNESS;
    light_config.level_control.options = (uint8_t)LevelControl::OptionsBitmap::kExecuteIfOff; // (uint8_t)LevelControl::OptionsBitmap::kCoupleColorTempToLevel | 
    
    light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    light_config.color_control.enhanced_color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    
    light_config.color_control_color_temperature.color_temp_physical_max_mireds = REMAP_TO_RANGE_INVERSE(CONFIG_COLOR_TEMP_WARM, MATTER_TEMPERATURE_FACTOR);
    light_config.color_control_color_temperature.color_temp_physical_min_mireds = REMAP_TO_RANGE_INVERSE(CONFIG_COLOR_TEMP_COLD, MATTER_TEMPERATURE_FACTOR);
    light_config.color_control_color_temperature.couple_color_temp_to_level_min_mireds = REMAP_TO_RANGE_INVERSE(CONFIG_COLOR_TEMP_COLD, MATTER_TEMPERATURE_FACTOR);
    light_config.color_control_color_temperature.start_up_color_temperature_mireds = nullptr;

    // endpoint handles can be used to add/modify clusters.
    endpoint_t *endpoint = color_temperature_light::create(node, &light_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create color temperature light endpoint"));
    
    light_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Light created with endpoint_id %d", light_endpoint_id);
    
    // Mark deferred persistence for some attributes that might be changed rapidly
    cluster_t *level_control_cluster = cluster::get(endpoint, LevelControl::Id);
    attribute_t *current_level_attribute = attribute::get(level_control_cluster, LevelControl::Attributes::CurrentLevel::Id);
    attribute::set_deferred_persistence(current_level_attribute);

    // Add brightness level min/max, if needed
    if (light_config.level_control_lighting.min_level != 1 ||
        light_config.level_control_lighting.min_level != MATTER_BRIGHTNESS) {
        esp_matter::attribute::create(level_control_cluster, LevelControl::Attributes::MinLevel::Id, MATTER_ATTRIBUTE_FLAG_READABLE, esp_matter_uint8(light_config.level_control_lighting.min_level));
        esp_matter::attribute::create(level_control_cluster, LevelControl::Attributes::MaxLevel::Id, MATTER_ATTRIBUTE_FLAG_READABLE, esp_matter_uint8(light_config.level_control_lighting.max_level));
    }

    cluster_t *color_control_cluster = cluster::get(endpoint, ColorControl::Id);
    attribute_t *color_temp_attribute = attribute::get(color_control_cluster, ColorControl::Attributes::ColorTemperatureMireds::Id);
    attribute::set_deferred_persistence(color_temp_attribute);
    
#if CONFIG_NIGHT_LED_CLUSTER
    esp_matter::endpoint::on_off_light::config_t night_light_config;
    night_light_config.on_off.on_off = DEFAULT_POWER;
    night_light_config.on_off_lighting.start_up_on_off = nullptr;
    endpoint_t *night_endpoint = esp_matter::endpoint::on_off_light::create(node, &night_light_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(night_endpoint != nullptr, ESP_LOGE(TAG, "Failed to create on/off light endpoint"));
    night_light_endpoint_id = endpoint::get_id(night_endpoint);
    ESP_LOGI(TAG, "Night light created with endpoint_id %d", night_light_endpoint_id);
#endif
}

/* Starting driver with default values */
void app_driver_restore_matter_state() {
    app_driver_light_set_defaults(light_endpoint_id);
#if CONFIG_NIGHT_LED_CLUSTER
    app_driver_night_led_set_defaults(night_light_endpoint_id);
#endif
}

// Button toggle callback
void button_toggle_cb()
{
    uint16_t endpoint_id = light_endpoint_id;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    attribute_t *attribute = attribute::get(endpoint_id, cluster_id, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);

    if (val.val.b) {
        cluster_id = LevelControl::Id;
        attribute_id = LevelControl::Attributes::CurrentLevel::Id;
        attribute = attribute::get(endpoint_id, cluster_id, attribute_id);
        attribute::get_val(attribute, &val);
        auto level = val.val.u8;
        val.val.u8 = 1;
        attribute::update(endpoint_id, cluster_id, attribute_id, &val);
        val.val.u8 = level;
        attribute::update(endpoint_id, cluster_id, attribute_id, &val);
    }
}

// Print hardware config
static void printHardwareConfig() {
    ESP_LOGI(TAG, "Warm led pin: %i", CONFIG_LED_WARM_GPIO);
    ESP_LOGI(TAG, "Cold led pin: %i", CONFIG_LED_COLD_GPIO);
#if CONFIG_NIGHT_LED_CLUSTER
    ESP_LOGI(TAG, "Night led pin: %i", CONFIG_NIGHT_LED_GPIO);
#endif
    ESP_LOGI(TAG, "Button pin: %i", CONFIG_BUTTON_GPIO);
#if CONFIG_INDICATOR_LED_INVERT
    ESP_LOGI(TAG, "Indicator led pin: %i (inverted)", CONFIG_INDICATOR_LED_GPIO);
#else
    ESP_LOGI(TAG, "Indicator led pin: %i", CONFIG_INDICATOR_LED_GPIO);
#endif
}

void app_driver_init() {
    printHardwareConfig();
    led_driver_init();
}
