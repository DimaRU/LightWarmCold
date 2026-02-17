/*
 Application main
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <common_macros.h>
#include "app_priv.h"
#include "indicator_driver.h"
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <setup_payload/OnboardingCodesUtil.h>
#include <setup_payload/SetupPayload.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        switch (event->InterfaceIpAddressChanged.Type)
        {
        case chip::DeviceLayer::InterfaceIpChangeType::kIpV4_Assigned:
            ESP_LOGI(TAG, "IPv4 assigned");
            break;
        case chip::DeviceLayer::InterfaceIpChangeType::kIpV4_Lost:
            ESP_LOGI(TAG, "IPv4 lost");
            break;
        case chip::DeviceLayer::InterfaceIpChangeType::kIpV6_Assigned:
            ESP_LOGI(TAG, "IPv6 assigned");
            break;
        case chip::DeviceLayer::InterfaceIpChangeType::kIpV6_Lost:
            ESP_LOGI(TAG, "IPv6 lost");
            break;
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kWiFiConnectivityChange:
        switch (event->WiFiConnectivityChange.Result)
        {
        case chip::DeviceLayer::ConnectivityChange::kConnectivity_Established:
            ESP_LOGI(TAG, "WiFi Connectivity established");
            signalIndicator(SignalIndicator::connected);
            break;
        case chip::DeviceLayer::ConnectivityChange::kConnectivity_Lost:
            ESP_LOGI(TAG, "WiFi Connectivity lost");
            break;
        case chip::DeviceLayer::ConnectivityChange::kConnectivity_NoChange:
            ESP_LOGI(TAG, "WiFi Connectivity no change");
            break;
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kThreadConnectivityChange:
        switch (event->ThreadConnectivityChange.Result)
        {
        case chip::DeviceLayer::ConnectivityChange::kConnectivity_Established:
            ESP_LOGI(TAG, "Thread Connectivity established");
            signalIndicator(SignalIndicator::connected);
            break;
        case chip::DeviceLayer::ConnectivityChange::kConnectivity_Lost:
            ESP_LOGI(TAG, "Thread Connectivity lost");
            break;
        case chip::DeviceLayer::ConnectivityChange::kConnectivity_NoChange:
            ESP_LOGI(TAG, "Thread Connectivity no change");
            break;
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kThreadInterfaceStateChange:
        ESP_LOGI(TAG, "Thread InterfaceState Change");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete, fabric count: %u", chip::Server::GetInstance().GetFabricTable().FabricCount());
        signalIndicator(SignalIndicator::commissioningStop);
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        signalIndicator(SignalIndicator::commissioningStop);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        signalIndicator(SignalIndicator::commissioningStart);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        signalIndicator(SignalIndicator::commissioningStop);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        signalIndicator(SignalIndicator::commissioningOpen);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        signalIndicator(SignalIndicator::commissioningClose);
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
    {
        ESP_LOGI(TAG, "Fabric removed successfully, left: %u", chip::Server::GetInstance().GetFabricTable().FabricCount());
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
        {
            ESP_LOGI(TAG, "Last fabric removed");
            // Initialise BLE manager
            CHIP_ERROR err = chip::DeviceLayer::Internal::BLEMgr().Init();
            if (err != CHIP_NO_ERROR)
            {
                ESP_LOGE(TAG, "BLEManager initialization failed: %" CHIP_ERROR_FORMAT, err.Format());
            }
            // Clear Wifi credentials if any
            if (chip::DeviceLayer::ConnectivityMgr().IsWiFiStationProvisioned())
            {
                ESP_LOGI(TAG, "ClearWiFiStationProvision");
                chip::DeviceLayer::ConnectivityMgr().ClearWiFiStationProvision();
            }
            // Clear Thread provision if any
            if (chip::DeviceLayer::ConnectivityMgr().IsThreadProvisioned())
            {
                ESP_LOGI(TAG, "ErasePersistentInfo");
                chip::DeviceLayer::ConnectivityMgr().ErasePersistentInfo();
            }
            esp_restart();
            // if (!commissionMgr.IsCommissioningWindowOpen())
            // {
            // Advertise over BLE
            // chip::CommissioningWindowManager & commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            // constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
            // chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
            // err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds);
            // if (err != CHIP_NO_ERROR) {
            //     ESP_LOGE(TAG, "Failed to open commissioning window: %" CHIP_ERROR_FORMAT, err.Format());
            // }
            // }
        }
        break;
    }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCHIPoBLEConnectionEstablished:
        ESP_LOGI(TAG, "BLE connection established");
        break;

    case chip::DeviceLayer::DeviceEventType::kCHIPoBLEConnectionClosed:
        ESP_LOGI(TAG, "BLE connection closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    }
}

static void button_reset_cb() {
    esp_matter::factory_reset();
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    signalIndicator(type == identification::START ? SignalIndicator::identificationStart : SignalIndicator::identificationStop);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, 
                                         uint16_t endpoint_id, 
                                         uint32_t cluster_id,
                                         uint32_t attribute_id, 
                                         esp_matter_attr_val_t *val, 
                                         void *priv_data)
{
    if (type == PRE_UPDATE) {
        app_driver_attribute_update(endpoint_id, cluster_id, attribute_id, val);
    }
    return ESP_OK;
}

static void setupLogging() {
    chip::Logging::SetLogRedirectCallback(&matterLoggingCallback);
    
    esp_log_level_set("chip[SVR]", ESP_LOG_ERROR);
    esp_log_level_set("chip[DMG]", ESP_LOG_ERROR);
    esp_log_level_set("chip[IN]", ESP_LOG_ERROR);
    esp_log_level_set("chip[TS]", ESP_LOG_ERROR);
    esp_log_level_set("chip[ZCL]", ESP_LOG_ERROR);
    esp_log_level_set("chip[EM]", ESP_LOG_ERROR);
    esp_log_level_set("chip[DL]", ESP_LOG_ERROR);
    esp_log_level_set("chip[IM]", ESP_LOG_ERROR);
    esp_log_level_set("chip[DIS]", ESP_LOG_ERROR);
    esp_log_level_set("chip[SC]", ESP_LOG_ERROR);

    esp_log_level_set("NimBLE", ESP_LOG_ERROR);
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);
    esp_log_level_set("ROUTE_HOOK", ESP_LOG_ERROR);
    esp_log_level_set("esp_matter_attribute", ESP_LOG_ERROR);
    esp_log_level_set("esp_matter_command", ESP_LOG_ERROR);
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    setupLogging();

#ifdef CONFIG_XIAO_ESP32C6_EXTERNAL_ANTENNA
    xiao_wifi_init();
#endif
    
    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    /* Initialize led driver */
    app_driver_init();
    indicator_driver_init();

    // Indicate start
    signalIndicator(SignalIndicator::startup);

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    
    // node handle can be used to add/modify other endpoints.
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));
    
    // Create endpoints
    app_driver_create_endpoints(node);

    // Install button driver
    app_driver_button_init(button_toggle_cb, button_reset_cb);


#if CHIP_DEVICE_CONFIG_ENABLE_THREAD && CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
    // Enable secondary network interface
    secondary_network_interface::config_t secondary_network_interface_config;
    endpoint = endpoint::secondary_network_interface::create(node, &secondary_network_interface_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create secondary network interface endpoint"));
#endif


#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    auto basic_information_cluster = cluster::get(chip::kRootEndpointId, BasicInformation::Id);

    esp_matter::cluster::basic_information::attribute::create_serial_number(basic_information_cluster, NULL, 0);
    esp_matter::cluster::basic_information::attribute::create_product_label(basic_information_cluster, NULL, 0);
    esp_matter::cluster::basic_information::attribute::create_product_url(basic_information_cluster, NULL, 0);
    
    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    auto serial_number_attr = attribute::get(basic_information_cluster, BasicInformation::Attributes::SerialNumber::Id);
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(serial_number_attr, &val);
    if (val.type == ESP_MATTER_VAL_TYPE_CHAR_STRING) {
        ESP_LOGI(TAG, "Device serial number: %.*s\n", val.val.a.s, val.val.a.b);
    }

    app_driver_restore_matter_state();

#if CONFIG_ENABLE_ENCRYPTED_OTA
    err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to initialized the encrypted OTA, err: %d", err));
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

    if (chip::DeviceLayer::ConfigurationMgr().IsFullyProvisioned()) {
        ESP_LOGI(TAG, "Fully provisioned");
    } else {
        ESP_LOGI(TAG, "Unprovisioned");
    }
    ESP_LOGI(TAG, "Fabric count %u", chip::Server::GetInstance().GetFabricTable().FabricCount());
    // PrintOnboardingCodes(chip::RendezvousInformationFlag::kBLE);

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
#if CONFIG_OPENTHREAD_CLI
    esp_matter::console::otcli_register_commands();
#endif
    esp_matter::console::init();
    ESP_LOGI(TAG, "Console initialized");
#endif
}
