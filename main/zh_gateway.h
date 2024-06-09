#pragma once

#include "stdio.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_eth.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "zh_json.h"
#include "zh_config.h"

#ifdef CONFIG_NETWORK_TYPE_DIRECT
#include "zh_espnow.h"
#define zh_send_message(a, b, c) zh_espnow_send(a, b, c)
#define ZH_EVENT ZH_ESPNOW
#else
#include "zh_network.h"
#define zh_send_message(a, b, c) zh_network_send(a, b, c)
#define ZH_EVENT ZH_NETWORK
#endif

#ifdef CONFIG_IDF_TARGET_ESP32
#define ZH_CHIP_TYPE HACHT_ESP32
#elif CONFIG_IDF_TARGET_ESP32S2
#define ZH_CHIP_TYPE HACHT_ESP32S2
#elif CONFIG_IDF_TARGET_ESP32S3
#define ZH_CHIP_TYPE HACHT_ESP32S3
#elif CONFIG_IDF_TARGET_ESP32C2
#define ZH_CHIP_TYPE HACHT_ESP32C2
#elif CONFIG_IDF_TARGET_ESP32C3
#define ZH_CHIP_TYPE HACHT_ESP32C3
#elif CONFIG_IDF_TARGET_ESP32C6
#define ZH_CHIP_TYPE HACHT_ESP32C6
#endif

#define ZH_LAN_MODULE_TYPE(x) esp_eth_phy_new_rtl8201(x) // Change it according to the LAN module used.
#define ZH_LAN_MODULE_POWER_PIN GPIO_NUM_12              // Change it according to the LAN module used.

#define ZH_WIFI_MAXIMUM_RETRY 5  // Maximum number of unsuccessful WiFi connection attempts.
#define ZH_WIFI_RECONNECT_TIME 5 // Waiting time (in seconds) between attempts to reconnect to WiFi (if number of attempts of unsuccessful connections is exceeded).
#define MAC_STR "%02X-%02X-%02X-%02X-%02X-%02X"

#define ZH_MESSAGE_TASK_PRIORITY 2 // Prioritize the task of sending messages to the MQTT.
#define ZH_MESSAGE_STACK_SIZE 3072 // The stack size of the task of sending messages to the MQTT.
#define ZH_SNTP_TASK_PRIORITY 2    // Prioritize the task to get the current time.
#define ZH_SNTP_STACK_SIZE 2048    // The stack size of the task to get the current time.
#define ZH_OTA_TASK_PRIORITY 3     // Prioritize the task of OTA updates.
#define ZH_OTA_STACK_SIZE 8192     // The stack size of the task of OTA updates.

#define ZH_MAX_GPIO_NUMBERS 48 // Maximum number of GPIOs on ESP modules.

typedef struct // Structure of data exchange between tasks, functions and event handlers.
{
    uint8_t self_mac[6];                          // Gateway MAC address. @note Depends at WiFi operation mode.
    bool sntp_is_enable;                          // SNTP client operation status flag. @note Used to control the SNTP functions when the network connection is established / lost.
    bool mqtt_is_enable;                          // MQTT client operation status flag. @note Used to control the MQTT functions when the network connection is established / lost.
    bool mqtt_is_connected;                       // MQTT broker connection status flag. @note Used to control the gateway system tasks when the MQTT connection is established / lost.
    esp_timer_handle_t wifi_reconnect_timer;      // Unique WiFi reconnection timer handle. @note Used when the number of attempts of unsuccessful connections is exceeded.
    uint8_t wifi_reconnect_retry_num;             // System counter for the number of unsuccessful WiFi connection attempts.
    esp_mqtt_client_handle_t mqtt_client;         // Unique MQTT client handle.
    TaskHandle_t gateway_attributes_message_task; // Unique task handle for zh_gateway_send_mqtt_json_attributes_message_task().
    TaskHandle_t gateway_keep_alive_message_task; // Unique task handle for zh_gateway_send_mqtt_json_keep_alive_message_task().
    TaskHandle_t gateway_current_time_task;       // Unique task handle for zh_send_espnow_current_time_task().
    struct                                        // Structure for initial transfer system data to the node update task.
    {
        zh_device_type_t device_type; // ESP-NOW device type.
        char app_name[32];            // Firmware application name.
        char app_version[32];         // Firmware application version.
        uint8_t mac_addr[6];          // ESP-NOW node MAC address.
    } espnow_ota_data;
    SemaphoreHandle_t espnow_ota_data_semaphore;    // Semaphore for control the acknowledgement of successful receipt of an update package from a node.
    SemaphoreHandle_t self_ota_in_progress_mutex;   // Mutex blocking the second run of the gateway update task.
    SemaphoreHandle_t espnow_ota_in_progress_mutex; // Mutex blocking the second run of the node update task.
} gateway_config_t;

extern const uint8_t server_certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t server_certificate_pem_end[] asm("_binary_certificate_pem_end");

#ifdef CONFIG_CONNECTION_TYPE_LAN
/**
 * @brief Function for LAN event processing.
 *
 * @param[in,out] arg Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
#else
/**
 * @brief Function for WiFi event processing.
 *
 * @param[in,out] arg Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
#endif
/**
 * @brief Function for ESP-NOW event processing
 *
 * @param[in,out] arg Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/**
 * @brief Function for MQTT event processing.
 *
 * @param[in,out] arg Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/**
 * @brief Task of updating the gateway firmware.
 *
 * @param[in,out] pvParameter Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_self_ota_update_task(void *pvParameter);

/**
 * @brief Task of updating the esp-now node firmware.
 *
 * @param[in,out] pvParameter Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_ota_update_task(void *pvParameter);

/**
 * @brief The task for getting the current time from the internet and sending it to the all esp-now nodes.
 *
 * @note To Do.
 *
 * @param[in,out] pvParameter Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_espnow_current_time_task(void *pvParameter);

/**
 * @brief Function for checking the correctness of the GPIO number value.
 *
 * @param gpio Value for check.
 *
 * @return GPIO number value or 0xFF if error
 */
uint8_t zh_gpio_number_check(const int gpio);

/**
 * @brief Function for checking the correctness of the bool variable value.
 *
 * @param value Value for check.
 *
 * @return Bool value
 */
bool zh_bool_value_check(const int value);

/**
 * @brief Function for checking the correctness of the sensor type value.
 *
 * @param type Value for check.
 *
 * @return Sensor type value or 0 if error
 */
uint8_t zh_sensor_type_check(const int type);

/**
 * @brief Function for checking the correctness of the uint16_t variable value.
 *
 * @param value Value for check.
 *
 * @return Uint16_t value or 60 if error
 */
uint16_t zh_uint16_value_check(const int value);

/**
 * @brief Task for prepare the attributes message from a gateway and transfer it to the processing function zh_espnow_send_mqtt_json_attributes_message().
 *
 * @param[in] pvParameter Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_gateway_send_mqtt_json_attributes_message_task(void *pvParameter);

/**
 * @brief Function for prepare the configuration message from a gateway and transfer it to the processing function zh_espnow_binary_sensor_send_mqtt_json_config_message().
 *
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_gateway_send_mqtt_json_config_message(const gateway_config_t *gateway_config);

/**
 * @brief Task for prepare the keep alive message from a gateway, transfer it to the processing function zh_espnow_send_mqtt_json_keep_alive_message() and sending to all esp-now nodes.
 *
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_gateway_send_mqtt_json_keep_alive_message_task(void *pvParameter);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the attributes message received from a esp-now node.
 *
 * @param[in,out] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_send_mqtt_json_attributes_message(zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the keep alive message received from a esp-now node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_send_mqtt_json_keep_alive_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the configuration message received from a zh_espnow_switch node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_switch_send_mqtt_json_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the hardware configuration message received from a zh_espnow_switch node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_switch_send_mqtt_json_hardware_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the status message received from a zh_espnow_switch node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_switch_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the configuration message received from a zh_espnow_led node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_led_send_mqtt_json_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the status message received from a zh_espnow_led node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_led_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the configuration message received from a zh_espnow_sensor node.
 *
 * @param[in,out] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_sensor_send_mqtt_json_config_message(zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the hardware configuration message received from a zh_espnow_sensor node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_sensor_send_mqtt_json_hardware_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the status message received from a zh_espnow_sensor node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_sensor_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the configuration message received from a zh_espnow_binary_sensor node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_binary_sensor_send_mqtt_json_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);

/**
 * @brief Function for converting to JSON and sending to the MQTT broker the status message received from a zh_espnow_binary_sensor node.
 *
 * @param[in] device_data Pointer to structure for data exchange between ESP-NOW devices.
 * @param[in] device_mac Pointer to ESP-NOW node MAC address.
 * @param[in] gateway_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_binary_sensor_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config);