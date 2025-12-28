/*
    On-board button driver
*/

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <esp_matter.h>
#include <common_macros.h>
#include <iot_button.h>
#include <button_gpio.h>
#include <app_priv.h>


using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "button_driver";

static bool perform_factory_reset = false;

static const button_config_t button_config = {
    .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
};

const button_gpio_config_t btn_gpio_cfg = {
    .gpio_num = CONFIG_BUTTON_GPIO,
    .active_level = 0,
};

static void app_driver_button_toggle_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Toggle button pressed");
    uint16_t endpoint_id = *(uint16_t *)data;
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


static void button_factory_reset_pressed_cb(void *arg, void *data)
{
    if (!perform_factory_reset) {
        ESP_LOGI(TAG, "Factory reset triggered. Release the button to start factory reset.");
        perform_factory_reset = true;
    }
}

static void button_factory_reset_released_cb(void *arg, void *data)
{
    if (perform_factory_reset) {
        ESP_LOGI(TAG, "Starting factory reset");
        esp_matter::factory_reset();
        perform_factory_reset = false;
    }
}

void app_driver_button_init(uint16_t *light_endpoint_id) {
	button_handle_t button_handle;
    esp_err_t err = iot_button_new_gpio_device(&button_config, &btn_gpio_cfg, &button_handle);
    ABORT_APP_ON_FAILURE(button_handle != nullptr, ESP_LOGE(TAG, "Failed to create button handle"));
	err |= iot_button_register_cb(button_handle, BUTTON_PRESS_DOWN, NULL, app_driver_button_toggle_cb, light_endpoint_id);
    err |= iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_HOLD, NULL, button_factory_reset_pressed_cb, NULL);
    err |= iot_button_register_cb(button_handle, BUTTON_PRESS_UP, NULL, button_factory_reset_released_cb, NULL);
    ESP_ERROR_CHECK(err);
}
