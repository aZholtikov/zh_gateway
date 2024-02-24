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
#include "zh_espnow.h"
#include "zh_json.h"
#include "zh_config.h"

#define ZH_LAN_MODULE_POWER_PIN GPIO_NUM_16
#define ZH_WIFI_MAXIMUM_RETRY 5
#define ZH_WIFI_RECONNECT_TIME 5
#define MAC_STR "%02X-%02X-%02X-%02X-%02X-%02X"

#define ZH_MESSAGE_TASK_PRIORITY 2
#define ZH_MESSAGE_STACK_SIZE 3072
#define ZH_SNTP_TASK_PRIORITY 2
#define ZH_SNTP_STACK_SIZE 2048
#define ZH_OTA_TASK_PRIORITY 3
#define ZH_OTA_STACK_SIZE 8192

extern const uint8_t server_certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t server_certificate_pem_end[] asm("_binary_certificate_pem_end");

static uint8_t s_self_mac[6] = {0};

static bool s_sntp_is_enable = false;
static bool s_mqtt_is_enable = false;
static bool s_mqtt_is_connected = false;

#if CONFIG_CONNECTION_TYPE_WIFI
static esp_timer_handle_t s_wifi_reconnect_timer = {0};
static uint8_t s_wifi_reconnect_retry_num = 0;
#endif

static bool s_espnow_self_ota_in_progress = false;
static bool s_espnow_ota_in_progress = false;
static uint16_t s_ota_message_part_number = 0;

static SemaphoreHandle_t s_espnow_data_semaphore = NULL;
static TaskHandle_t s_gateway_attributes_message_task = {0};
static TaskHandle_t s_gateway_keep_alive_message_task = {0};
static TaskHandle_t s_gateway_current_time_task = {0};

static esp_mqtt_client_handle_t s_mqtt_client = {0};

#if CONFIG_CONNECTION_TYPE_LAN
static void s_zh_eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
#endif
#if CONFIG_CONNECTION_TYPE_WIFI
static void s_zh_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
#endif
static void s_zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void s_zh_mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void s_zh_self_ota_update_task(void *pvParameter);
static void s_zh_espnow_ota_update_task(void *pvParameter);

static void s_zh_send_espnow_current_time_task(void *pvParameter);

static void s_zh_gateway_send_mqtt_json_attributes_message_task(void *pvParameter);
static void s_zh_gateway_send_mqtt_json_config_message(void);
static void s_zh_gateway_send_mqtt_json_keep_alive_message_task(void *pvParameter);

static void s_zh_espnow_send_mqtt_json_attributes_message(zh_espnow_data_t *device_data, uint8_t *device_mac);
static void s_zh_espnow_send_mqtt_json_keep_alive_message(zh_espnow_data_t *device_data, uint8_t *device_mac);

static void s_zh_espnow_switch_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac);
static void s_zh_espnow_switch_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac);

static void s_zh_espnow_led_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac);
static void s_zh_espnow_led_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac);

static void s_zh_espnow_sensor_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac);
static void s_zh_espnow_sensor_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac);

static void s_zh_espnow_binary_sensor_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac);
static void s_zh_espnow_binary_sensor_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac);

void app_main(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = {0};
    esp_ota_get_state_partition(running, &ota_state);
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
#if CONFIG_CONNECTION_TYPE_LAN
    gpio_config_t config = {0};
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_OUTPUT;
    config.pin_bit_mask = (1ULL << ZH_LAN_MODULE_POWER_PIN);
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&config);
    gpio_set_level(ZH_LAN_MODULE_POWER_PIN, HIGH);
    esp_netif_config_t esp_netif_config = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *esp_netif_eth = esp_netif_new(&esp_netif_config);
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    esp_eth_config_t esp_eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t esp_eth_handle = NULL;
    esp_eth_driver_install(&esp_eth_config, &esp_eth_handle);
    esp_netif_attach(esp_netif_eth, esp_eth_new_netif_glue(esp_eth_handle));
    esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &s_zh_eth_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &s_zh_eth_event_handler, NULL, NULL);
    esp_eth_start(esp_eth_handle);
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
    esp_wifi_start();
    esp_read_mac(s_self_mac, ESP_MAC_WIFI_STA);
#endif
#if CONFIG_CONNECTION_TYPE_WIFI
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_init_sta_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_sta_config);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID_NAME,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &s_zh_wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &s_zh_wifi_event_handler, NULL, NULL);
    esp_wifi_start();
    esp_read_mac(s_self_mac, ESP_MAC_WIFI_SOFTAP);
#endif
    zh_espnow_init_config_t zh_espnow_init_config = ZH_ESPNOW_INIT_CONFIG_DEFAULT();
#if CONFIG_CONNECTION_TYPE_WIFI
    zh_espnow_init_config.wifi_interface = WIFI_IF_AP;
#endif
    zh_espnow_init(&zh_espnow_init_config);
    esp_event_handler_instance_register(ZH_ESPNOW, ESP_EVENT_ANY_ID, &s_zh_espnow_event_handler, NULL, NULL);
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        vTaskDelay(60000 / portTICK_PERIOD_MS);
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

#if CONFIG_CONNECTION_TYPE_LAN
static void s_zh_eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ETHERNET_EVENT_DISCONNECTED:
        if (s_mqtt_is_enable == true)
        {
            esp_mqtt_client_stop(s_mqtt_client);
            s_mqtt_is_enable = false;
        }
        if (s_sntp_is_enable == true)
        {
            esp_sntp_stop();
            s_sntp_is_enable = false;
            vTaskDelete(s_gateway_current_time_task);
        }
        break;
    case IP_EVENT_ETH_GOT_IP:
        if (s_mqtt_is_enable == false)
        {
            esp_mqtt_client_config_t mqtt_config = {
                .broker.address.uri = CONFIG_MQTT_BROKER_URL,
                .buffer.size = 2048,
            };
            s_mqtt_client = esp_mqtt_client_init(&mqtt_config);
            esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, s_zh_mqtt_event_handler, NULL);
            esp_mqtt_client_start(s_mqtt_client);
            s_mqtt_is_enable = true;
        }
        if (s_sntp_is_enable == false)
        {
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
            esp_sntp_setservername(0, CONFIG_NTP_SERVER_URL);
            esp_sntp_init();
            s_sntp_is_enable = true;
            xTaskCreatePinnedToCore(&s_zh_send_espnow_current_time_task, "s_zh_send_espnow_current_time_task", ZH_SNTP_STACK_SIZE, NULL, ZH_SNTP_TASK_PRIORITY, &s_gateway_current_time_task, tskNO_AFFINITY);
        }
        break;
    default:
        break;
    }
}
#endif

#if CONFIG_CONNECTION_TYPE_WIFI
static void s_zh_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        if (s_mqtt_is_enable == true)
        {
            esp_mqtt_client_stop(s_mqtt_client);
            s_mqtt_is_enable = false;
        }
        if (s_sntp_is_enable == true)
        {
            esp_sntp_stop();
            s_sntp_is_enable = false;
            vTaskDelete(s_gateway_current_time_task);
        }
        if (s_wifi_reconnect_retry_num < ZH_WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            ++s_wifi_reconnect_retry_num;
        }
        else
        {
            s_wifi_reconnect_retry_num = 0;
            esp_timer_create_args_t wifi_reconnect_timer_args = {
                .callback = (void *)esp_wifi_connect};
            esp_timer_create(&wifi_reconnect_timer_args, &s_wifi_reconnect_timer);
            esp_timer_start_once(s_wifi_reconnect_timer, ZH_WIFI_RECONNECT_TIME * 1000);
        }
        break;
    case IP_EVENT_STA_GOT_IP:
        s_wifi_reconnect_retry_num = 0;
        if (s_mqtt_is_enable == false)
        {
            esp_mqtt_client_config_t mqtt_config = {
                .broker.address.uri = CONFIG_MQTT_BROKER_URL,
                .buffer.size = 2048,
            };
            s_mqtt_client = esp_mqtt_client_init(&mqtt_config);
            esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, s_zh_mqtt_event_handler, NULL);
            esp_mqtt_client_start(s_mqtt_client);
            s_mqtt_is_enable = true;
        }
        if (s_sntp_is_enable == false)
        {
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
            esp_sntp_setservername(0, CONFIG_NTP_SERVER_URL);
            esp_sntp_init();
            s_sntp_is_enable = true;
            xTaskCreatePinnedToCore(&s_zh_send_espnow_current_time_task, "s_zh_send_espnow_current_time_task", ZH_SNTP_STACK_SIZE, NULL, ZH_SNTP_TASK_PRIORITY, &s_gateway_current_time_task, tskNO_AFFINITY);
        }
        break;
    default:
        break;
    }
}
#endif

static void s_zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ZH_ESPNOW_ON_RECV_EVENT:
        zh_espnow_event_on_recv_t *recv_data = event_data;
        if (recv_data->data_len != sizeof(zh_espnow_data_t))
        {
            goto ZH_ESPNOW_EVENT_HANDLER_EXIT;
        }
        zh_espnow_data_t data = {0};
        memcpy(&data, recv_data->data, recv_data->data_len);
        char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(data.device_type)) + 20);
        sprintf(topic, "%s/%s/" MAC_STR, CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(data.device_type), MAC2STR(recv_data->mac_addr));
        switch (data.payload_type)
        {
        case ZHPT_ATTRIBUTES:
            s_zh_espnow_send_mqtt_json_attributes_message(&data, recv_data->mac_addr);
            break;
        case ZHPT_KEEP_ALIVE:
            s_zh_espnow_send_mqtt_json_keep_alive_message(&data, recv_data->mac_addr);
            break;
        case ZHPT_CONFIG:
            switch (data.device_type)
            {
            case ZHDT_SWITCH:
                s_zh_espnow_switch_send_mqtt_json_config_message(&data, recv_data->mac_addr);
                break;
            case ZHDT_LED:
                s_zh_espnow_led_send_mqtt_json_config_message(&data, recv_data->mac_addr);
                break;
            case ZHDT_SENSOR:
                s_zh_espnow_sensor_send_mqtt_json_config_message(&data, recv_data->mac_addr);
                break;
            case ZHDT_BINARY_SENSOR:
                s_zh_espnow_binary_sensor_send_mqtt_json_config_message(&data, recv_data->mac_addr);
                break;
            default:
                break;
            }
            break;
        case ZHPT_STATE:
            switch (data.device_type)
            {
            case ZHDT_SWITCH:
                s_zh_espnow_switch_send_mqtt_json_status_message(&data, recv_data->mac_addr);
                break;
            case ZHDT_LED:
                s_zh_espnow_led_send_mqtt_json_status_message(&data, recv_data->mac_addr);
                break;
            case ZHDT_SENSOR:
                s_zh_espnow_sensor_send_mqtt_json_status_message(&data, recv_data->mac_addr);
                break;
            case ZHDT_BINARY_SENSOR:
                s_zh_espnow_binary_sensor_send_mqtt_json_status_message(&data, recv_data->mac_addr);
                break;
            default:
                break;
            }
            break;
        case ZHPT_UPDATE:
            zh_espnow_ota_data_t espnow_ota_data = {0};
            s_espnow_ota_in_progress = true;
            memcpy(&espnow_ota_data.chip_type, &data.payload_data.espnow_ota_message.chip_type, sizeof(data.payload_data.espnow_ota_message.chip_type));
            memcpy(&espnow_ota_data.device_type, &data.device_type, sizeof(data.device_type));
            memcpy(&espnow_ota_data.app_name, &data.payload_data.espnow_ota_message.app_name, sizeof(data.payload_data.espnow_ota_message.app_name));
            memcpy(&espnow_ota_data.app_version, &data.payload_data.espnow_ota_message.app_version, sizeof(data.payload_data.espnow_ota_message.app_version));
            memcpy(&espnow_ota_data.mac_addr, recv_data->mac_addr, sizeof(espnow_ota_data.mac_addr));
            xTaskCreatePinnedToCore(&s_zh_espnow_ota_update_task, "s_zh_espnow_ota_update_task", ZH_OTA_STACK_SIZE, &espnow_ota_data, ZH_OTA_TASK_PRIORITY, NULL, tskNO_AFFINITY);
            break;
        case ZHPT_UPDATE_PROGRESS:
            xSemaphoreGive(s_espnow_data_semaphore);
            break;
        case ZHPT_UPDATE_FAIL:
            esp_mqtt_client_publish(s_mqtt_client, topic, "update_fail", 0, 2, true);
            break;
        case ZHPT_UPDATE_SUCCESS:
            esp_mqtt_client_publish(s_mqtt_client, topic, "update_success", 0, 2, true);
            break;
        default:
            break;
        }
        free(topic);
    ZH_ESPNOW_EVENT_HANDLER_EXIT:
        free(recv_data->data);
        break;
    case ZH_ESPNOW_ON_SEND_EVENT:
        break;
    default:
        break;
    }
}

static void s_zh_mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        if (s_mqtt_is_connected == false)
        {
            char *topic_for_subscribe = NULL;
            char *supported_device_type = NULL;
            for (zh_device_type_t i = 1; i <= ZHDT_MAX; ++i)
            {
                supported_device_type = zh_get_device_type_value_name(i);
                topic_for_subscribe = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(supported_device_type) + 4);
                sprintf(topic_for_subscribe, "%s/%s/#", CONFIG_MQTT_TOPIC_PREFIX, supported_device_type);
                esp_mqtt_client_subscribe(s_mqtt_client, topic_for_subscribe, 2);
                free(topic_for_subscribe);
            }
            s_zh_gateway_send_mqtt_json_config_message();
            xTaskCreatePinnedToCore(&s_zh_gateway_send_mqtt_json_attributes_message_task, "s_zh_gateway_send_mqtt_json_attributes_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_gateway_attributes_message_task, tskNO_AFFINITY);
            xTaskCreatePinnedToCore(&s_zh_gateway_send_mqtt_json_keep_alive_message_task, "s_zh_gateway_send_mqtt_json_keep_alive_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_gateway_keep_alive_message_task, tskNO_AFFINITY);
        }
        s_mqtt_is_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        if (s_mqtt_is_connected == true)
        {
            vTaskDelete(s_gateway_attributes_message_task);
            vTaskDelete(s_gateway_keep_alive_message_task);
            zh_keep_alive_message_t keep_alive_message = {0};
            keep_alive_message.online_status = ZH_OFFLINE;
            zh_espnow_data_t data = {0};
            data.device_type = ZHDT_GATEWAY;
            data.payload_type = ZHPT_KEEP_ALIVE;
            data.payload_data = (zh_payload_data_t)keep_alive_message;
            zh_espnow_send(NULL, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        }
        s_mqtt_is_connected = false;
        break;
    case MQTT_EVENT_DATA:
        char incoming_topic[(event->topic_len) + 1];
        memset(&incoming_topic, 0, sizeof(incoming_topic));
        strncat(incoming_topic, event->topic, event->topic_len);
        uint8_t incoming_data_mac[6] = {0};
        zh_device_type_t incoming_data_device_type = ZHDT_NONE;
        zh_payload_type_t incoming_data_payload_type = ZHPT_NONE;
        char *extracted_topic_data = strtok(incoming_topic, "/"); // Extract topic prefix.
        if (extracted_topic_data == NULL)
        {
            break;
        }
        extracted_topic_data = strtok(NULL, "/"); // Extract device type.
        if (extracted_topic_data == NULL)
        {
            break;
        }
        for (zh_device_type_t i = 1; i < ZHDT_MAX; ++i)
        {
            if (strncmp(extracted_topic_data, zh_get_device_type_value_name(i), strlen(extracted_topic_data) + 1) == 0)
            {
                incoming_data_device_type = i;
                break;
            }
        }
        extracted_topic_data = strtok(NULL, "/"); // Extract MAC address.
        if (extracted_topic_data == NULL)
        {
            break;
        }
        sscanf(extracted_topic_data, "%hhX-%hhX-%hhX-%hhX-%hhX-%hhX", &incoming_data_mac[0], &incoming_data_mac[1], &incoming_data_mac[2], &incoming_data_mac[3], &incoming_data_mac[4], &incoming_data_mac[5]);
        extracted_topic_data = strtok(NULL, "/"); // Extract payload type.
        if (extracted_topic_data != NULL)
        {
            for (zh_payload_type_t i = 1; i < ZHPT_MAX; ++i)
            {
                if (strncmp(extracted_topic_data, zh_get_payload_type_value_name(i), strlen(extracted_topic_data) + 1) == 0)
                {
                    incoming_data_payload_type = i;
                    break;
                }
            }
        }
        char incoming_payload[(event->data_len) + 1];
        memset(&incoming_payload, 0, sizeof(incoming_payload));
        strncat(incoming_payload, event->data, event->data_len);
        zh_espnow_data_t data = {0};
        data.device_type = ZHDT_GATEWAY;
        zh_status_message_t status_message = {0};
        zh_switch_status_message_t switch_status_message = {0};
        zh_led_status_message_t led_status_message = {0};
        switch (incoming_data_device_type)
        {
        case ZHDT_GATEWAY:
            if (incoming_data_payload_type == ZHPT_NONE)
            {
                if (memcmp(&s_self_mac, &incoming_data_mac, 6) == 0)
                {
                    if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && s_espnow_self_ota_in_progress == false)
                    {
                        s_espnow_self_ota_in_progress = true;
                        xTaskCreatePinnedToCore(&s_zh_self_ota_update_task, "s_zh_self_ota_update_task", ZH_OTA_STACK_SIZE, NULL, ZH_OTA_TASK_PRIORITY, NULL, tskNO_AFFINITY);
                    }
                    else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                    {
                        esp_restart();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            break;
        case ZHDT_SWITCH:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && s_espnow_ota_in_progress == false)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else
                {
                    break;
                }
                break;
            case ZHPT_SET:
                for (ha_on_off_type_t i = 1; i < HAONOFT_MAX; ++i)
                {
                    if (strncmp(incoming_payload, zh_get_on_off_type_value_name(i), strlen(incoming_payload) + 1) == 0)
                    {
                        switch_status_message.status = i;
                        status_message = (zh_status_message_t)switch_status_message;
                        data.payload_type = ZHPT_SET;
                        data.payload_data = (zh_payload_data_t)status_message;
                        zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                        break;
                    }
                }
                break;
            default:
                break;
            }
            break;
        case ZHDT_LED:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && s_espnow_ota_in_progress == false)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else
                {
                    break;
                }
                break;
            case ZHPT_SET:
                for (ha_on_off_type_t i = 1; i < HAONOFT_MAX; ++i)
                {
                    if (strncmp(incoming_payload, zh_get_on_off_type_value_name(i), strlen(incoming_payload) + 1) == 0)
                    {
                        led_status_message.status = i;
                        status_message = (zh_status_message_t)led_status_message;
                        data.payload_type = ZHPT_SET;
                        data.payload_data = (zh_payload_data_t)status_message;
                        zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                        break;
                    }
                }
                break;
            case ZHPT_BRIGHTNESS:
                led_status_message.brightness = atoi(incoming_payload);
                status_message = (zh_status_message_t)led_status_message;
                data.payload_type = ZHPT_BRIGHTNESS;
                data.payload_data = (zh_payload_data_t)status_message;
                zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_TEMPERATURE:
                led_status_message.temperature = atoi(incoming_payload);
                status_message = (zh_status_message_t)led_status_message;
                data.payload_type = ZHPT_TEMPERATURE;
                data.payload_data = (zh_payload_data_t)status_message;
                zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_RGB:
                char *extracted_rgb_data = strtok(incoming_payload, ","); // Extract red value.
                if (extracted_rgb_data == NULL)
                {
                    break;
                }
                led_status_message.red = atoi(extracted_rgb_data);
                extracted_rgb_data = strtok(NULL, ","); // Extract green value.
                if (extracted_rgb_data == NULL)
                {
                    break;
                }
                led_status_message.green = atoi(extracted_rgb_data);
                extracted_rgb_data = strtok(NULL, ","); // Extract blue value.
                if (extracted_rgb_data == NULL)
                {
                    break;
                }
                led_status_message.blue = atoi(extracted_rgb_data);
                status_message = (zh_status_message_t)led_status_message;
                data.payload_type = ZHPT_RGB;
                data.payload_data = (zh_payload_data_t)status_message;
                zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            default:
                break;
            }
            break;
        case ZHDT_SENSOR:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && s_espnow_ota_in_progress == false)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else
                {
                    break;
                }
                break;
            default:
                break;
            }
            break;
        case ZHDT_BINARY_SENSOR:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && s_espnow_ota_in_progress == false)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_espnow_send(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else
                {
                    break;
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    }
}

static void s_zh_self_ota_update_task(void *pvParameter)
{
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(ZHDT_GATEWAY)) + 20);
    sprintf(topic, "%s/%s/" MAC_STR, CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(ZHDT_GATEWAY), MAC2STR(s_self_mac));
    char self_ota_write_data[1025] = {0};
    esp_ota_handle_t update_handle = {0};
    const esp_partition_t *update_partition = NULL;
    esp_mqtt_client_publish(s_mqtt_client, topic, "update_begin", 0, 2, true);
    const esp_app_desc_t *app_info = esp_app_get_description();
    char *app_name = (char *)calloc(1, strlen(CONFIG_FIRMWARE_UPGRADE_URL) + strlen(app_info->project_name) + 6);
    sprintf(app_name, "%s/%s.bin", CONFIG_FIRMWARE_UPGRADE_URL, app_info->project_name);
    esp_http_client_config_t config = {
        .url = app_name,
        .cert_pem = (char *)server_certificate_pem_start,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t https_client = esp_http_client_init(&config);
    free(app_name);
    esp_http_client_open(https_client, 0);
    esp_http_client_fetch_headers(https_client);
    update_partition = esp_ota_get_next_update_partition(NULL);
    esp_mqtt_client_publish(s_mqtt_client, topic, "update_progress", 0, 2, true);
    esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    for (;;)
    {
        int data_read_size = esp_http_client_read(https_client, self_ota_write_data, 1024);
        if (data_read_size < 0)
        {
            esp_http_client_close(https_client);
            esp_http_client_cleanup(https_client);
            s_espnow_self_ota_in_progress = false;
            esp_mqtt_client_publish(s_mqtt_client, topic, "update_error", 0, 2, true);
            free(topic);
            vTaskDelete(NULL);
        }
        else if (data_read_size > 0)
        {
            esp_ota_write(update_handle, (const void *)self_ota_write_data, data_read_size);
        }
        else
        {
            break;
        }
    }
    if (esp_ota_end(update_handle) != ESP_OK)
    {
        esp_http_client_close(https_client);
        esp_http_client_cleanup(https_client);
        s_espnow_self_ota_in_progress = false;
        esp_mqtt_client_publish(s_mqtt_client, topic, "update_fail", 0, 2, true);
        free(topic);
        vTaskDelete(NULL);
    }
    esp_ota_set_boot_partition(update_partition);
    esp_http_client_close(https_client);
    esp_http_client_cleanup(https_client);
    esp_mqtt_client_publish(s_mqtt_client, topic, "update_success", 0, 2, true);
    free(topic);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
}

static void s_zh_espnow_ota_update_task(void *pvParameter)
{
    s_espnow_data_semaphore = xSemaphoreCreateBinary();
    zh_espnow_ota_data_t *espnow_ota_data = pvParameter;
    zh_espnow_ota_message_t espnow_ota_message = {0};
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(espnow_ota_data->device_type)) + 20);
    sprintf(topic, "%s/%s/" MAC_STR, CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(espnow_ota_data->device_type), MAC2STR(espnow_ota_data->mac_addr));
    esp_mqtt_client_publish(s_mqtt_client, topic, "update_begin", 0, 2, true);
    char espnow_ota_write_data[sizeof(espnow_ota_message.data) + 1] = {0};
    char *app_name = (char *)calloc(1, strlen(CONFIG_FIRMWARE_UPGRADE_URL) + strlen(espnow_ota_data->app_name) + 6);
    sprintf(app_name, "%s/%s.bin", CONFIG_FIRMWARE_UPGRADE_URL, espnow_ota_data->app_name);
    esp_http_client_config_t config = {
        .url = app_name,
        .cert_pem = (char *)server_certificate_pem_start,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t https_client = esp_http_client_init(&config);
    free(app_name);
    esp_http_client_open(https_client, 0);
    esp_http_client_fetch_headers(https_client);
    data.payload_type = ZHPT_UPDATE_BEGIN;
    zh_espnow_send(espnow_ota_data->mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
    xSemaphoreTake(s_espnow_data_semaphore, 5000 / portTICK_PERIOD_MS);
    esp_mqtt_client_publish(s_mqtt_client, topic, "update_progress", 0, 2, true);
    for (;;)
    {
        int data_read_size = esp_http_client_read(https_client, espnow_ota_write_data, sizeof(espnow_ota_message.data));
        if (data_read_size < 0)
        {
            esp_http_client_close(https_client);
            esp_http_client_cleanup(https_client);
            s_espnow_ota_in_progress = false;
            esp_mqtt_client_publish(s_mqtt_client, topic, "update_error", 0, 2, true);
            free(topic);
            vTaskDelete(NULL);
        }
        else if (data_read_size > 0)
        {
            ++s_ota_message_part_number;
            espnow_ota_message.data_len = data_read_size;
            espnow_ota_message.part = s_ota_message_part_number;
            memcpy(&espnow_ota_message.data, &espnow_ota_write_data, data_read_size);
            data.payload_type = ZHPT_UPDATE_PROGRESS;
            data.payload_data = (zh_payload_data_t)espnow_ota_message;
            zh_espnow_send(espnow_ota_data->mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
            if (xSemaphoreTake(s_espnow_data_semaphore, 1000 / portTICK_PERIOD_MS) != pdTRUE)
            {
                esp_http_client_close(https_client);
                esp_http_client_cleanup(https_client);
                data.payload_type = ZHPT_UPDATE_ERROR;
                zh_espnow_send(espnow_ota_data->mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                s_espnow_ota_in_progress = false;
                s_ota_message_part_number = 0;
                esp_mqtt_client_publish(s_mqtt_client, topic, "update_error", 0, 2, true);
                free(topic);
                vTaskDelete(NULL);
            }
        }
        else
        {
            esp_http_client_close(https_client);
            esp_http_client_cleanup(https_client);
            data.payload_type = ZHPT_UPDATE_END;
            zh_espnow_send(espnow_ota_data->mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
            s_espnow_ota_in_progress = false;
            s_ota_message_part_number = 0;
            esp_mqtt_client_publish(s_mqtt_client, topic, "update_end", 0, 2, true);
            free(topic);
            vTaskDelete(NULL);
        }
    }
}

static void s_zh_send_espnow_current_time_task(void *pvParameter)
{
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    time_t now;
    setenv("TZ", CONFIG_NTP_TIME_ZONE, 1);
    tzset();
    for (;;)
    {
        time(&now);
        // To Do.
        vTaskDelay(86400000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_gateway_send_mqtt_json_attributes_message_task(void *pvParameter)
{
    const esp_app_desc_t *app_info = esp_app_get_description();
    zh_attributes_message_t attributes_message = {0};
    attributes_message.chip_type = HACHT_ESP32;
    strcpy(attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    attributes_message.cpu_frequency = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    attributes_message.reset_reason = (uint8_t)esp_reset_reason();
    strcpy(attributes_message.app_name, app_info->project_name);
    strcpy(attributes_message.app_version, app_info->version);
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    data.payload_type = ZHPT_ATTRIBUTES;
    for (;;)
    {
        attributes_message.heap_size = esp_get_free_heap_size();
        attributes_message.min_heap_size = esp_get_minimum_free_heap_size();
        attributes_message.uptime = esp_timer_get_time() / 1000000;
        data.payload_data = (zh_payload_data_t)attributes_message;
        s_zh_espnow_send_mqtt_json_attributes_message(&data, s_self_mac);
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_gateway_send_mqtt_json_config_message(void)
{
    zh_binary_sensor_config_message_t binary_sensor_config_message = {0};
    binary_sensor_config_message.unique_id = 1;
    binary_sensor_config_message.binary_sensor_device_class = HABSDC_CONNECTIVITY;
    binary_sensor_config_message.payload_on = HAONOFT_CONNECT;
    binary_sensor_config_message.payload_off = HAONOFT_NONE;
    binary_sensor_config_message.expire_after = 30;
    binary_sensor_config_message.enabled_by_default = true;
    binary_sensor_config_message.force_update = true;
    binary_sensor_config_message.qos = 2;
    binary_sensor_config_message.retain = true;
    zh_config_message_t config_message = {0};
    config_message = (zh_config_message_t)binary_sensor_config_message;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    data.payload_type = ZHPT_CONFIG;
    data.payload_data = (zh_payload_data_t)config_message;
    s_zh_espnow_binary_sensor_send_mqtt_json_config_message(&data, s_self_mac);
}

static void s_zh_gateway_send_mqtt_json_keep_alive_message_task(void *pvParameter)
{
    zh_keep_alive_message_t keep_alive_message = {0};
    keep_alive_message.online_status = ZH_ONLINE;
    zh_binary_sensor_status_message_t binary_sensor_status_message = {0};
    binary_sensor_status_message.sensor_type = HAST_GATEWAY;
    binary_sensor_status_message.connect = HAONOFT_CONNECT;
    zh_status_message_t status_message = {0};
    status_message = (zh_status_message_t)binary_sensor_status_message;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    for (;;)
    {
        data.payload_type = ZHPT_KEEP_ALIVE;
        data.payload_data = (zh_payload_data_t)keep_alive_message;
        zh_espnow_send(NULL, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        data.payload_type = ZHPT_STATE;
        data.payload_data = (zh_payload_data_t)status_message;
        s_zh_espnow_binary_sensor_send_mqtt_json_status_message(&data, s_self_mac);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_espnow_send_mqtt_json_attributes_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    uint32_t secs = device_data->payload_data.attributes_message.uptime;
    uint32_t mins = secs / 60;
    uint32_t hours = mins / 60;
    uint32_t days = hours / 24;
    char *mac = (char *)calloc(1, 18);
    sprintf(mac, "" MAC_STR "", MAC2STR(device_mac));
    char *uptime = (char *)calloc(1, 44);
    sprintf(uptime, "Days:%lu Hours:%lu Mins:%lu", days, hours - (days * 24), mins - (hours * 60));
    char *cpu_frequency = calloc(1, 4);
    sprintf(cpu_frequency, "%d", device_data->payload_data.attributes_message.cpu_frequency);
    char *heap_size = calloc(1, 7);
    sprintf(heap_size, "%lu", device_data->payload_data.attributes_message.heap_size);
    char *min_heap_size = calloc(1, 7);
    sprintf(min_heap_size, "%lu", device_data->payload_data.attributes_message.min_heap_size);
    char *reset_reason = calloc(1, 4);
    sprintf(reset_reason, "%d", device_data->payload_data.attributes_message.reset_reason);
    zh_json_t json;
    char buffer[512] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "Type", zh_get_device_type_value_name(device_data->device_type));
    zh_json_add(&json, "MAC", mac);
    zh_json_add(&json, "Chip", zh_get_chip_type_value_name(device_data->payload_data.attributes_message.chip_type));
    if (device_data->payload_data.attributes_message.sensor_type != 0)
    {
        zh_json_add(&json, "Sensor", zh_get_sensor_type_value_name(device_data->payload_data.attributes_message.sensor_type));
    }
    zh_json_add(&json, "CPU frequency", cpu_frequency);
    zh_json_add(&json, "Flash size", device_data->payload_data.attributes_message.flash_size);
    zh_json_add(&json, "Current heap size", heap_size);
    zh_json_add(&json, "Minimum heap size", min_heap_size);
    zh_json_add(&json, "Last reset reason", reset_reason);
    zh_json_add(&json, "App name", device_data->payload_data.attributes_message.app_name);
    zh_json_add(&json, "App version", device_data->payload_data.attributes_message.app_version);
    zh_json_add(&json, "Uptime", uptime);
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(topic, "%s/%s/" MAC_STR "/attributes", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(mac);
    free(uptime);
    free(cpu_frequency);
    free(heap_size);
    free(min_heap_size);
    free(reset_reason);
    free(topic);
}

static void s_zh_espnow_send_mqtt_json_keep_alive_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *status = (device_data->payload_data.keep_alive_message.online_status == ZH_ONLINE) ? "online" : "offline";
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(topic, "%s/%s/" MAC_STR "/status", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(s_mqtt_client, topic, status, 0, 2, true);
    free(topic);
}

static void s_zh_espnow_switch_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *name = (char *)calloc(1, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)calloc(1, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.switch_config_message.unique_id);
    char *state_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *availability_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(availability_topic, "%s/%s/" MAC_STR "/status", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *command_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24);
    sprintf(command_topic, "%s/%s/" MAC_STR "/set", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *json_attributes_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)calloc(1, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.switch_config_message.qos);
    zh_json_t json;
    char buffer[512] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "platform", "mqtt");
    zh_json_add(&json, "name", name);
    zh_json_add(&json, "unique_id", unique_id);
    zh_json_add(&json, "device_class", zh_get_switch_device_class_value_name(device_data->payload_data.config_message.switch_config_message.device_class));
    zh_json_add(&json, "state_topic", state_topic);
    zh_json_add(&json, "value_template", "{{ value_json.state }}");
    zh_json_add(&json, "availability_topic", availability_topic);
    zh_json_add(&json, "command_topic", command_topic);
    zh_json_add(&json, "json_attributes_topic", json_attributes_topic);
    zh_json_add(&json, "enabled_by_default", (device_data->payload_data.config_message.switch_config_message.enabled_by_default == true) ? "true" : "false");
    zh_json_add(&json, "optimistic", (device_data->payload_data.config_message.switch_config_message.optimistic == true) ? "true" : "false");
    zh_json_add(&json, "payload_on", zh_get_on_off_type_value_name(device_data->payload_data.config_message.switch_config_message.payload_on));
    zh_json_add(&json, "payload_off", zh_get_on_off_type_value_name(device_data->payload_data.config_message.switch_config_message.payload_off));
    zh_json_add(&json, "qos", qos);
    zh_json_add(&json, "retain", (device_data->payload_data.config_message.switch_config_message.retain == true) ? "true" : "false");
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(unique_id) + 16);
    sprintf(topic, "%s/switch/%s/config", CONFIG_MQTT_TOPIC_PREFIX, unique_id);
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(name);
    free(unique_id);
    free(state_topic);
    free(availability_topic);
    free(command_topic);
    free(json_attributes_topic);
    free(qos);
    free(topic);
}

static void s_zh_espnow_switch_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    zh_json_t json;
    char buffer[128] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "state", zh_get_on_off_type_value_name(device_data->payload_data.status_message.switch_status_message.status));
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(topic);
}

static void s_zh_espnow_led_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *name = (char *)calloc(1, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)calloc(1, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.led_config_message.unique_id);
    char *state_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *availability_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(availability_topic, "%s/%s/" MAC_STR "/status", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *command_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24);
    sprintf(command_topic, "%s/%s/" MAC_STR "/set", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *json_attributes_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)calloc(1, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.led_config_message.qos);
    char *brightness_command_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(brightness_command_topic, "%s/%s/" MAC_STR "/brightness", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *color_temp_command_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 32);
    sprintf(color_temp_command_topic, "%s/%s/" MAC_STR "/temperature", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *rgb_command_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24);
    sprintf(rgb_command_topic, "%s/%s/" MAC_STR "/rgb", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    zh_json_t json;
    char buffer[1024] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "platform", "mqtt");
    zh_json_add(&json, "name", name);
    zh_json_add(&json, "unique_id", unique_id);
    zh_json_add(&json, "state_topic", state_topic);
    zh_json_add(&json, "state_value_template", "{{ value_json.state }}");
    zh_json_add(&json, "availability_topic", availability_topic);
    zh_json_add(&json, "command_topic", command_topic);
    zh_json_add(&json, "json_attributes_topic", json_attributes_topic);
    zh_json_add(&json, "enabled_by_default", (device_data->payload_data.config_message.led_config_message.enabled_by_default == true) ? "true" : "false");
    zh_json_add(&json, "optimistic", (device_data->payload_data.config_message.led_config_message.optimistic == true) ? "true" : "false");
    zh_json_add(&json, "payload_on", zh_get_on_off_type_value_name(device_data->payload_data.config_message.led_config_message.payload_on));
    zh_json_add(&json, "payload_off", zh_get_on_off_type_value_name(device_data->payload_data.config_message.led_config_message.payload_off));
    zh_json_add(&json, "qos", qos);
    zh_json_add(&json, "retain", (device_data->payload_data.config_message.led_config_message.retain == true) ? "true" : "false");
    zh_json_add(&json, "brightness_state_topic", state_topic);
    zh_json_add(&json, "brightness_value_template", "{{ value_json.brightness }}");
    zh_json_add(&json, "brightness_command_topic", brightness_command_topic);
    if (device_data->payload_data.config_message.led_config_message.led_type == HALT_WW || device_data->payload_data.config_message.led_config_message.led_type == HALT_RGBWW)
    {
        zh_json_add(&json, "color_temp_state_topic", state_topic);
        zh_json_add(&json, "color_temp_value_template", "{{ value_json.temperature }}");
        zh_json_add(&json, "color_temp_command_topic", color_temp_command_topic);
    }
    if (device_data->payload_data.config_message.led_config_message.led_type == HALT_RGB || device_data->payload_data.config_message.led_config_message.led_type == HALT_RGBW || device_data->payload_data.config_message.led_config_message.led_type == HALT_RGBWW)
    {
        zh_json_add(&json, "rgb_state_topic", state_topic);
        zh_json_add(&json, "rgb_value_template", "{{ value_json.rgb | join(',') }}");
        zh_json_add(&json, "rgb_command_topic", rgb_command_topic);
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(unique_id) + 15);
    sprintf(topic, "%s/light/%s/config", CONFIG_MQTT_TOPIC_PREFIX, unique_id);
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(name);
    free(unique_id);
    free(state_topic);
    free(availability_topic);
    free(command_topic);
    free(json_attributes_topic);
    free(qos);
    free(brightness_command_topic);
    free(color_temp_command_topic);
    free(rgb_command_topic);
    free(topic);
}

static void s_zh_espnow_led_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *brightness = (char *)calloc(1, 4);
    sprintf(brightness, "%d", device_data->payload_data.status_message.led_status_message.brightness);
    char *temperature = (char *)calloc(1, 6);
    sprintf(temperature, "%d", device_data->payload_data.status_message.led_status_message.temperature);
    char *rgb = (char *)calloc(1, 12);
    sprintf(rgb, "%d,%d,%d", device_data->payload_data.status_message.led_status_message.red, device_data->payload_data.status_message.led_status_message.green, device_data->payload_data.status_message.led_status_message.blue);
    zh_json_t json;
    char buffer[128] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "state", zh_get_on_off_type_value_name(device_data->payload_data.status_message.led_status_message.status));
    zh_json_add(&json, "brightness", brightness);
    zh_json_add(&json, "temperature", temperature);
    zh_json_add(&json, "rgb", rgb);
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(topic);
}

static void s_zh_espnow_sensor_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *name = (char *)calloc(1, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)calloc(1, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.sensor_config_message.unique_id);
    char *state_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *value_template = (char *)calloc(1, strlen(zh_get_sensor_device_class_value_name(device_data->payload_data.config_message.sensor_config_message.sensor_device_class)) + 18);
    sprintf(value_template, "{{ value_json.%s }}", zh_get_sensor_device_class_value_name(device_data->payload_data.config_message.sensor_config_message.sensor_device_class));
    char *json_attributes_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)calloc(1, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.sensor_config_message.qos);
    char *expire_after = (char *)calloc(1, 6);
    sprintf(expire_after, "%d", device_data->payload_data.config_message.sensor_config_message.expire_after);
    char *suggested_display_precision = (char *)calloc(1, 4);
    sprintf(suggested_display_precision, "%d", device_data->payload_data.config_message.sensor_config_message.suggested_display_precision);
    zh_json_t json;
    char buffer[512] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "platform", "mqtt");
    zh_json_add(&json, "name", name);
    zh_json_add(&json, "unique_id", unique_id);
    zh_json_add(&json, "device_class", zh_get_sensor_device_class_value_name(device_data->payload_data.config_message.sensor_config_message.sensor_device_class));
    zh_json_add(&json, "state_topic", state_topic);
    zh_json_add(&json, "value_template", value_template);
    zh_json_add(&json, "json_attributes_topic", json_attributes_topic);
    zh_json_add(&json, "enabled_by_default", (device_data->payload_data.config_message.sensor_config_message.enabled_by_default == true) ? "true" : "false");
    zh_json_add(&json, "qos", qos);
    zh_json_add(&json, "retain", (device_data->payload_data.config_message.sensor_config_message.retain == true) ? "true" : "false");
    zh_json_add(&json, "expire_after", expire_after);
    zh_json_add(&json, "force_update", (device_data->payload_data.config_message.sensor_config_message.force_update == true) ? "true" : "false");
    zh_json_add(&json, "suggested_display_precision", suggested_display_precision);
    zh_json_add(&json, "unit_of_measurement", device_data->payload_data.config_message.sensor_config_message.unit_of_measurement);
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(unique_id) + 16);
    sprintf(topic, "%s/sensor/%s/config", CONFIG_MQTT_TOPIC_PREFIX, unique_id);
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(name);
    free(unique_id);
    free(state_topic);
    free(value_template);
    free(json_attributes_topic);
    free(qos);
    free(expire_after);
    free(suggested_display_precision);
    free(topic);
}

static void s_zh_espnow_sensor_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *temperature = (char *)calloc(1, 317);
    zh_json_t json;
    char buffer[512] = {0};
    zh_json_init(&json);
    switch (device_data->payload_data.status_message.sensor_status_message.sensor_type)
    {
    case HAST_DS18B20:
        sprintf(temperature, "%f", device_data->payload_data.status_message.sensor_status_message.temperature);
        zh_json_add(&json, "temperature", temperature);
        break;
    default:
        break;
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(temperature);
    free(topic);
}

static void s_zh_espnow_binary_sensor_send_mqtt_json_config_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    char *name = (char *)calloc(1, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)calloc(1, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.binary_sensor_config_message.unique_id);
    char *state_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *value_template = (char *)calloc(1, strlen(zh_get_binary_sensor_device_class_value_name(device_data->payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class)) + 18);
    sprintf(value_template, "{{ value_json.%s }}", zh_get_binary_sensor_device_class_value_name(device_data->payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class));
    char *json_attributes_topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)calloc(1, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.binary_sensor_config_message.qos);
    char *expire_after = (char *)calloc(1, 6);
    sprintf(expire_after, "%d", device_data->payload_data.config_message.binary_sensor_config_message.expire_after);
    char *off_delay = (char *)calloc(1, 6);
    sprintf(off_delay, "%d", device_data->payload_data.config_message.binary_sensor_config_message.off_delay);
    zh_json_t json;
    char buffer[512] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "platform", "mqtt");
    zh_json_add(&json, "name", name);
    zh_json_add(&json, "unique_id", unique_id);
    zh_json_add(&json, "device_class", zh_get_binary_sensor_device_class_value_name(device_data->payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class));
    zh_json_add(&json, "state_topic", state_topic);
    zh_json_add(&json, "value_template", value_template);
    zh_json_add(&json, "json_attributes_topic", json_attributes_topic);
    zh_json_add(&json, "enabled_by_default", (device_data->payload_data.config_message.binary_sensor_config_message.enabled_by_default == true) ? "true" : "false");
    zh_json_add(&json, "payload_on", zh_get_on_off_type_value_name(device_data->payload_data.config_message.binary_sensor_config_message.payload_on));
    zh_json_add(&json, "payload_off", zh_get_on_off_type_value_name(device_data->payload_data.config_message.binary_sensor_config_message.payload_off));
    zh_json_add(&json, "qos", qos);
    zh_json_add(&json, "retain", (device_data->payload_data.config_message.binary_sensor_config_message.retain == true) ? "true" : "false");
    if (device_data->payload_data.config_message.binary_sensor_config_message.expire_after != 0)
    {
        zh_json_add(&json, "expire_after", expire_after);
    }
    zh_json_add(&json, "force_update", (device_data->payload_data.config_message.binary_sensor_config_message.force_update == true) ? "true" : "false");
    if (device_data->payload_data.config_message.binary_sensor_config_message.off_delay != 0)
    {
        zh_json_add(&json, "off_delay", off_delay);
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(unique_id) + 23);
    sprintf(topic, "%s/binary_sensor/%s/config", CONFIG_MQTT_TOPIC_PREFIX, unique_id);
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(name);
    free(unique_id);
    free(state_topic);
    free(value_template);
    free(json_attributes_topic);
    free(qos);
    free(expire_after);
    free(off_delay);
    free(topic);
}

static void s_zh_espnow_binary_sensor_send_mqtt_json_status_message(zh_espnow_data_t *device_data, uint8_t *device_mac)
{
    zh_json_t json;
    char buffer[512] = {0};
    zh_json_init(&json);
    switch (device_data->payload_data.status_message.binary_sensor_status_message.sensor_type)
    {
    case HAST_GATEWAY:
        zh_json_add(&json, "connectivity", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.connect));
        break;
    case HAST_WINDOW:
        zh_json_add(&json, "window", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.open));
        zh_json_add(&json, "battery", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.battery));
        break;
    case HAST_DOOR:
        zh_json_add(&json, "door", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.open));
        zh_json_add(&json, "battery", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.battery));
        break;
    case HAST_LEAKAGE:
        zh_json_add(&json, "moisture", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.leakage));
        zh_json_add(&json, "battery", zh_get_on_off_type_value_name(device_data->payload_data.status_message.binary_sensor_status_message.battery));
        break;
    default:
        break;
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)calloc(1, strlen(CONFIG_MQTT_TOPIC_PREFIX) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(topic, "%s/%s/" MAC_STR "/state", CONFIG_MQTT_TOPIC_PREFIX, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(s_mqtt_client, topic, buffer, 0, 2, true);
    free(topic);
}