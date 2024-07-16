#include "zh_gateway.h"

gateway_config_t gateway_main_config = {0};

void app_main(void)
{
    gateway_config_t *gateway_config = &gateway_main_config;
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    zh_load_config(gateway_config);
    if (gateway_config->software_config.is_lan_mode == true)
    {
        gpio_config_t config = {0};
        config.intr_type = GPIO_INTR_DISABLE;
        config.mode = GPIO_MODE_OUTPUT;
        config.pin_bit_mask = (1ULL << ZH_LAN_MODULE_POWER_PIN);
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&config);
        gpio_set_level(ZH_LAN_MODULE_POWER_PIN, 1);
        esp_netif_config_t esp_netif_config = ESP_NETIF_DEFAULT_ETH();
        esp_netif_t *esp_netif_eth = esp_netif_new(&esp_netif_config);
        eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
        eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
        eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
        esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
        esp_eth_phy_t *phy = ZH_LAN_MODULE_TYPE(&phy_config);
        esp_eth_config_t esp_eth_config = ETH_DEFAULT_CONFIG(mac, phy);
        esp_eth_handle_t esp_eth_handle = NULL;
        esp_eth_driver_install(&esp_eth_config, &esp_eth_handle);
        esp_netif_attach(esp_netif_eth, esp_eth_new_netif_glue(esp_eth_handle));
        esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &zh_eth_event_handler, gateway_config, NULL);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &zh_eth_event_handler, gateway_config, NULL);
        esp_eth_start(esp_eth_handle);
        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&wifi_init_config);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
        esp_wifi_start();
        esp_read_mac(gateway_config->self_mac, ESP_MAC_WIFI_STA);
    }
    else
    {
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t wifi_init_sta_config = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&wifi_init_sta_config);
        wifi_config_t wifi_config = {0};
        memcpy(wifi_config.sta.ssid, gateway_config->software_config.ssid_name, strlen(gateway_config->software_config.ssid_name));
        memcpy(wifi_config.sta.password, gateway_config->software_config.ssid_password, strlen(gateway_config->software_config.ssid_password));
        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &zh_wifi_event_handler, gateway_config, NULL);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &zh_wifi_event_handler, gateway_config, NULL);
        esp_wifi_start();
        esp_read_mac(gateway_config->self_mac, ESP_MAC_WIFI_SOFTAP);
    }
#ifdef CONFIG_NETWORK_TYPE_DIRECT
    zh_espnow_init_config_t zh_espnow_init_config = ZH_ESPNOW_INIT_CONFIG_DEFAULT();
    zh_espnow_init_config.queue_size = 128;
    if (gateway_config->software_config.is_lan_mode == false)
    {
        zh_espnow_init_config.wifi_interface = WIFI_IF_AP;
    }
    zh_espnow_init(&zh_espnow_init_config);
    esp_event_handler_instance_register(ZH_EVENT, ESP_EVENT_ANY_ID, &zh_espnow_event_handler, gateway_config, NULL);
#else
    zh_network_init_config_t zh_network_init_config = ZH_NETWORK_INIT_CONFIG_DEFAULT();
    zh_network_init_config.queue_size = 128;
    if (gateway_config->software_config.is_lan_mode == false)
    {
        zh_network_init_config.wifi_interface = WIFI_IF_AP;
    }
    zh_network_init(&zh_network_init_config);
    esp_event_handler_instance_register(ZH_EVENT, ESP_EVENT_ANY_ID, &zh_espnow_event_handler, gateway_config, NULL);
#endif
    gateway_config->espnow_ota_data_semaphore = xSemaphoreCreateBinary();
    gateway_config->self_ota_in_progress_mutex = xSemaphoreCreateMutex();
    gateway_config->espnow_ota_in_progress_mutex = xSemaphoreCreateMutex();
    gateway_config->device_check_in_progress_mutex = xSemaphoreCreateMutex();
    zh_vector_init(&gateway_config->available_device_vector, sizeof(available_device_t), false);
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = {0};
    esp_ota_get_state_partition(running, &ota_state);
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        vTaskDelay(60000 / portTICK_PERIOD_MS);
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

void zh_load_config(gateway_config_t *gateway_config)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("config", NVS_READWRITE, &nvs_handle);
    uint8_t config_is_present = 0;
    if (nvs_get_u8(nvs_handle, "present", &config_is_present) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(nvs_handle, "present", 0xFE);
        nvs_close(nvs_handle);
    SETUP_INITIAL_SETTINGS:
#ifdef CONFIG_CONNECTION_TYPE_LAN
        gateway_config->software_config.is_lan_mode = true;
        strcpy(gateway_config->software_config.ssid_name, "NULL");
        strcpy(gateway_config->software_config.ssid_password, "NULL");
#else
        gateway_config->software_config.is_lan_mode = false;
        strcpy(gateway_config->software_config.ssid_name, CONFIG_WIFI_SSID_NAME);
        strcpy(gateway_config->software_config.ssid_password, CONFIG_WIFI_PASSWORD);
#endif
        strcpy(gateway_config->software_config.mqtt_broker_url, CONFIG_MQTT_BROKER_URL);
        strcpy(gateway_config->software_config.mqtt_topic_prefix, CONFIG_MQTT_TOPIC_PREFIX);
#ifdef CONFIG_SYSLOG_SERVER_USING
        gateway_config->software_config.is_syslog_server_usage = true;
        strcpy(gateway_config->software_config.syslog_server_ip, CONFIG_SYSLOG_SERVER_IP);
        gateway_config->software_config.syslog_server_port = CONFIG_SYSLOG_SERVER_PORT;
#else
        gateway_config->software_config.is_syslog_server_usage = false;
        strcpy(gateway_config->software_config.syslog_server_ip, "NULL");
        gateway_config->software_config.syslog_server_port = 0;
#endif
#ifdef CONFIG_NTP_SERVER_USING
        gateway_config->software_config.is_ntp_server_usage = true;
        strcpy(gateway_config->software_config.ntp_server_url, CONFIG_NTP_SERVER_URL);
        strcpy(gateway_config->software_config.ntp_time_zone, CONFIG_NTP_TIME_ZONE);
#else
        gateway_config->software_config.is_ntp_server_usage = false;
        strcpy(gateway_config->software_config.ntp_server_url, "NULL");
        strcpy(gateway_config->software_config.ntp_time_zone, "NULL");
#endif
#ifdef CONFIG_FIRMWARE_UPGRADE_SERVER_USING
        gateway_config->software_config.is_ota_server_usage = true;
        strcpy(gateway_config->software_config.firmware_upgrade_url, CONFIG_FIRMWARE_UPGRADE_URL);
#else
        gateway_config->software_config.is_ota_server_usage = false;
        strcpy(gateway_config->software_config.firmware_upgrade_url, "NULL");
#endif
        zh_save_config(gateway_config);
        return;
    }
    esp_err_t err = ESP_OK;
    size_t size = 0;
    err += nvs_get_u8(nvs_handle, "lan_mode", (uint8_t *)&gateway_config->software_config.is_lan_mode);
    err += nvs_get_str(nvs_handle, "ssid_name", NULL, &size);
    err += nvs_get_str(nvs_handle, "ssid_name", gateway_config->software_config.ssid_name, &size);
    err += nvs_get_str(nvs_handle, "ssid_password", NULL, &size);
    err += nvs_get_str(nvs_handle, "ssid_password", gateway_config->software_config.ssid_password, &size);
    err += nvs_get_str(nvs_handle, "mqtt_url", NULL, &size);
    err += nvs_get_str(nvs_handle, "mqtt_url", gateway_config->software_config.mqtt_broker_url, &size);
    err += nvs_get_str(nvs_handle, "topic_prefix", NULL, &size);
    err += nvs_get_str(nvs_handle, "topic_prefix", gateway_config->software_config.mqtt_topic_prefix, &size);
    err += nvs_get_u8(nvs_handle, "syslog_usage", (uint8_t *)&gateway_config->software_config.is_syslog_server_usage);
    err += nvs_get_str(nvs_handle, "syslog_ip", NULL, &size);
    err += nvs_get_str(nvs_handle, "syslog_ip", gateway_config->software_config.syslog_server_ip, &size);
    err += nvs_get_u32(nvs_handle, "syslog_port", &gateway_config->software_config.syslog_server_port);
    err += nvs_get_u8(nvs_handle, "ntp_usage", (uint8_t *)&gateway_config->software_config.is_ntp_server_usage);
    err += nvs_get_str(nvs_handle, "ntp_url", NULL, &size);
    err += nvs_get_str(nvs_handle, "ntp_url", gateway_config->software_config.ntp_server_url, &size);
    err += nvs_get_str(nvs_handle, "time_zone", NULL, &size);
    err += nvs_get_str(nvs_handle, "time_zone", gateway_config->software_config.ntp_time_zone, &size);
    err += nvs_get_u8(nvs_handle, "ota_usage", (uint8_t *)&gateway_config->software_config.is_ota_server_usage);
    err += nvs_get_str(nvs_handle, "upgrade_url", NULL, &size);
    err += nvs_get_str(nvs_handle, "upgrade_url", gateway_config->software_config.firmware_upgrade_url, &size);
    nvs_close(nvs_handle);
    if (err != ESP_OK)
    {
        goto SETUP_INITIAL_SETTINGS;
    }
}

void zh_save_config(const gateway_config_t *gateway_config)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("config", NVS_READWRITE, &nvs_handle);
    nvs_set_u8(nvs_handle, "lan_mode", gateway_config->software_config.is_lan_mode);
    nvs_set_str(nvs_handle, "ssid_name", gateway_config->software_config.ssid_name);
    nvs_set_str(nvs_handle, "ssid_password", gateway_config->software_config.ssid_password);
    nvs_set_str(nvs_handle, "mqtt_url", gateway_config->software_config.mqtt_broker_url);
    nvs_set_str(nvs_handle, "topic_prefix", gateway_config->software_config.mqtt_topic_prefix);
    nvs_set_u8(nvs_handle, "syslog_usage", gateway_config->software_config.is_syslog_server_usage);
    nvs_set_str(nvs_handle, "syslog_ip", gateway_config->software_config.syslog_server_ip);
    nvs_set_u32(nvs_handle, "syslog_port", gateway_config->software_config.syslog_server_port);
    nvs_set_u8(nvs_handle, "ntp_usage", gateway_config->software_config.is_ntp_server_usage);
    nvs_set_str(nvs_handle, "ntp_url", gateway_config->software_config.ntp_server_url);
    nvs_set_str(nvs_handle, "time_zone", gateway_config->software_config.ntp_time_zone);
    nvs_set_u8(nvs_handle, "ota_usage", gateway_config->software_config.is_ota_server_usage);
    nvs_set_str(nvs_handle, "upgrade_url", gateway_config->software_config.firmware_upgrade_url);
    nvs_close(nvs_handle);
}

void zh_eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gateway_config_t *gateway_config = arg;
    switch (event_id)
    {
    case ETHERNET_EVENT_DISCONNECTED:
        if (gateway_config->mqtt_is_enable == true)
        {
            esp_mqtt_client_stop(gateway_config->mqtt_client);
            gateway_config->mqtt_is_enable = false;
        }
        if (gateway_config->software_config.is_ntp_server_usage == true)
        {
            if (gateway_config->sntp_is_enable == true)
            {
                esp_sntp_stop();
                gateway_config->sntp_is_enable = false;
                vTaskDelete(gateway_config->gateway_current_time_task);
            }
        }
        if (gateway_config->software_config.is_syslog_server_usage == true)
        {
            if (gateway_config->syslog_is_enable == true)
            {
                zh_syslog_deinit();
                gateway_config->syslog_is_enable = false;
            }
        }
        break;
    case IP_EVENT_ETH_GOT_IP:
        if (gateway_config->mqtt_is_enable == false)
        {
            esp_mqtt_client_config_t mqtt_config = {0};
            mqtt_config.buffer.size = 2048;
            mqtt_config.broker.address.uri = gateway_config->software_config.mqtt_broker_url;
            gateway_config->mqtt_client = esp_mqtt_client_init(&mqtt_config);
            esp_mqtt_client_register_event(gateway_config->mqtt_client, ESP_EVENT_ANY_ID, zh_mqtt_event_handler, gateway_config);
            esp_mqtt_client_start(gateway_config->mqtt_client);
            gateway_config->mqtt_is_enable = true;
        }
        if (gateway_config->software_config.is_ntp_server_usage == true)
        {
            if (gateway_config->sntp_is_enable == false)
            {
                esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
                esp_sntp_setservername(0, gateway_config->software_config.ntp_server_url);
                esp_sntp_init();
                gateway_config->sntp_is_enable = true;
                xTaskCreatePinnedToCore(&zh_send_espnow_current_time_task, "NULL", ZH_SNTP_STACK_SIZE, gateway_config, ZH_SNTP_TASK_PRIORITY, (TaskHandle_t *)&gateway_config->gateway_current_time_task, tskNO_AFFINITY);
            }
        }
        if (gateway_config->software_config.is_syslog_server_usage == true)
        {
            zh_syslog_init_config_t syslog_init_config = ZH_SYSLOG_INIT_CONFIG_DEFAULT();
            memcpy(syslog_init_config.syslog_ip, gateway_config->software_config.syslog_server_ip, strlen(gateway_config->software_config.syslog_server_ip));
            syslog_init_config.syslog_port = gateway_config->software_config.syslog_server_port;
            zh_syslog_init(&syslog_init_config);
            gateway_config->syslog_is_enable = true;
        }
        break;
    default:
        break;
    }
}

void zh_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gateway_config_t *gateway_config = arg;
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        if (gateway_config->mqtt_is_enable == true)
        {
            esp_mqtt_client_stop(gateway_config->mqtt_client);
            gateway_config->mqtt_is_enable = false;
        }
        if (gateway_config->software_config.is_ntp_server_usage == true)
        {
            if (gateway_config->sntp_is_enable == true)
            {
                esp_sntp_stop();
                gateway_config->sntp_is_enable = false;
                vTaskDelete(gateway_config->gateway_current_time_task);
            }
        }
        if (gateway_config->software_config.is_syslog_server_usage == true)
        {
            if (gateway_config->syslog_is_enable == true)
            {
                zh_syslog_deinit();
                gateway_config->syslog_is_enable = false;
            }
        }
        if (gateway_config->wifi_reconnect_retry_num < ZH_WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            ++gateway_config->wifi_reconnect_retry_num;
        }
        else
        {
            gateway_config->wifi_reconnect_retry_num = 0;
            esp_timer_create_args_t wifi_reconnect_timer_args = {
                .callback = (void *)esp_wifi_connect};
            esp_timer_create(&wifi_reconnect_timer_args, &gateway_config->wifi_reconnect_timer);
            esp_timer_start_once(gateway_config->wifi_reconnect_timer, ZH_WIFI_RECONNECT_TIME * 1000);
        }
        break;
    case IP_EVENT_STA_GOT_IP:
        gateway_config->wifi_reconnect_retry_num = 0;
        if (gateway_config->mqtt_is_enable == false)
        {
            esp_mqtt_client_config_t mqtt_config = {0};
            mqtt_config.buffer.size = 2048;
            mqtt_config.broker.address.uri = gateway_config->software_config.mqtt_broker_url;
            gateway_config->mqtt_client = esp_mqtt_client_init(&mqtt_config);
            esp_mqtt_client_register_event(gateway_config->mqtt_client, ESP_EVENT_ANY_ID, zh_mqtt_event_handler, gateway_config);
            esp_mqtt_client_start(gateway_config->mqtt_client);
            gateway_config->mqtt_is_enable = true;
        }
        if (gateway_config->software_config.is_ntp_server_usage == true)
        {
            if (gateway_config->sntp_is_enable == false)
            {
                esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
                esp_sntp_setservername(0, gateway_config->software_config.ntp_server_url);
                esp_sntp_init();
                gateway_config->sntp_is_enable = true;
                xTaskCreatePinnedToCore(&zh_send_espnow_current_time_task, "NULL", ZH_SNTP_STACK_SIZE, gateway_config, ZH_SNTP_TASK_PRIORITY, (TaskHandle_t *)&gateway_config->gateway_current_time_task, tskNO_AFFINITY);
            }
        }
        if (gateway_config->software_config.is_syslog_server_usage == true)
        {
            zh_syslog_init_config_t syslog_init_config = ZH_SYSLOG_INIT_CONFIG_DEFAULT();
            memcpy(syslog_init_config.syslog_ip, gateway_config->software_config.syslog_server_ip, strlen(gateway_config->software_config.syslog_server_ip));
            syslog_init_config.syslog_port = gateway_config->software_config.syslog_server_port;
            zh_syslog_init(&syslog_init_config);
            gateway_config->syslog_is_enable = true;
        }
        break;
    default:
        break;
    }
}

void zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gateway_config_t *gateway_config = arg;
    switch (event_id)
    {
#ifdef CONFIG_NETWORK_TYPE_DIRECT
    case ZH_ESPNOW_ON_RECV_EVENT:
        zh_espnow_event_on_recv_t *recv_data = event_data;
        if (recv_data->data_len != sizeof(zh_espnow_data_t))
        {
            goto ZH_ESPNOW_EVENT_HANDLER_EXIT;
        }
#else
    case ZH_NETWORK_ON_RECV_EVENT:
        zh_network_event_on_recv_t *recv_data = event_data;
        if (recv_data->data_len != sizeof(zh_espnow_data_t))
        {
            goto ZH_NETWORK_EVENT_HANDLER_EXIT;
        }
#endif
        zh_espnow_data_t *data = (zh_espnow_data_t *)recv_data->data;
        char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(data->device_type)) + 20, MALLOC_CAP_8BIT);
        memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(data->device_type)) + 20);
        sprintf(topic, "%s/%s/" MAC_STR, gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(data->device_type), MAC2STR(recv_data->mac_addr));
        switch (data->payload_type)
        {
        case ZHPT_ATTRIBUTES:
            zh_espnow_send_mqtt_json_attributes_message(data, recv_data->mac_addr, gateway_config);
            break;
        case ZHPT_KEEP_ALIVE:
            if (xSemaphoreTake(gateway_config->device_check_in_progress_mutex, portTICK_PERIOD_MS) == pdTRUE)
            {
                bool is_found = false;
                for (uint16_t i = 0; i < zh_vector_get_size(&gateway_config->available_device_vector); ++i)
                {
                    available_device_t *available_device = zh_vector_get_item(&gateway_config->available_device_vector, i);
                    if (memcmp(recv_data->mac_addr, available_device->mac_addr, 6) == 0)
                    {
                        zh_vector_delete_item(&gateway_config->available_device_vector, i);
                        is_found = true;
                    }
                }
                if (gateway_config->syslog_is_enable == true && is_found == false)
                {
                    char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                    memset(mac, 0, 18);
                    sprintf(mac, "" MAC_STR "", MAC2STR(recv_data->mac_addr));
                    zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(data->device_type), "Connected to gateway.");
                    heap_caps_free(mac);
                }
                available_device_t available_device = {0};
                available_device.device_type = data->device_type;
                memcpy(available_device.mac_addr, recv_data->mac_addr, 6);
                available_device.frequency = data->payload_data.keep_alive_message.message_frequency;
                available_device.time = esp_timer_get_time() / 1000000;
                zh_vector_push_back(&gateway_config->available_device_vector, &available_device);
                xSemaphoreGive(gateway_config->device_check_in_progress_mutex);
            }
            zh_espnow_send_mqtt_json_keep_alive_message(data, recv_data->mac_addr, gateway_config);
            break;
        case ZHPT_CONFIG:
            switch (data->device_type)
            {
            case ZHDT_SWITCH:
                zh_espnow_switch_send_mqtt_json_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_LED:
                zh_espnow_led_send_mqtt_json_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_SENSOR:
                zh_espnow_sensor_send_mqtt_json_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_BINARY_SENSOR:
                zh_espnow_binary_sensor_send_mqtt_json_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            default:
                break;
            }
            break;
        case ZHPT_HARDWARE:
            switch (data->device_type)
            {
            case ZHDT_SWITCH:
                zh_espnow_switch_send_mqtt_json_hardware_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_LED:
                zh_espnow_led_send_mqtt_json_hardware_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_SENSOR:
                zh_espnow_sensor_send_mqtt_json_hardware_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_BINARY_SENSOR:
                zh_espnow_binary_sensor_send_mqtt_json_hardware_config_message(data, recv_data->mac_addr, gateway_config);
                break;
            default:
                break;
            }
            break;
        case ZHPT_STATE:
            switch (data->device_type)
            {
            case ZHDT_SWITCH:
                zh_espnow_switch_send_mqtt_json_status_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_LED:
                zh_espnow_led_send_mqtt_json_status_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_SENSOR:
                zh_espnow_sensor_send_mqtt_json_status_message(data, recv_data->mac_addr, gateway_config);
                break;
            case ZHDT_BINARY_SENSOR:
                zh_espnow_binary_sensor_send_mqtt_json_status_message(data, recv_data->mac_addr, gateway_config);
                break;
            default:
                break;
            }
            break;
        case ZHPT_UPDATE:
            if (xSemaphoreTake(gateway_config->espnow_ota_in_progress_mutex, portTICK_PERIOD_MS) == pdTRUE)
            {
                gateway_config->espnow_ota_data.device_type = data->device_type;
                memcpy(gateway_config->espnow_ota_data.app_name, data->payload_data.ota_message.espnow_ota_data.app_name, sizeof(data->payload_data.ota_message.espnow_ota_data.app_name));
                memcpy(gateway_config->espnow_ota_data.app_version, data->payload_data.ota_message.espnow_ota_data.app_version, sizeof(data->payload_data.ota_message.espnow_ota_data.app_version));
                memcpy(gateway_config->espnow_ota_data.mac_addr, recv_data->mac_addr, 6);
                xTaskCreatePinnedToCore(&zh_espnow_ota_update_task, "NULL", ZH_OTA_STACK_SIZE, gateway_config, ZH_OTA_TASK_PRIORITY, NULL, tskNO_AFFINITY);
                xSemaphoreGive(gateway_config->espnow_ota_in_progress_mutex);
            }
            break;
        case ZHPT_UPDATE_PROGRESS:
            xSemaphoreGive(gateway_config->espnow_ota_data_semaphore);
            break;
        case ZHPT_UPDATE_FAIL:
            esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_fail", 0, 2, true);
            if (gateway_config->syslog_is_enable == true)
            {
                char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                memset(mac, 0, 18);
                sprintf(mac, "" MAC_STR "", MAC2STR(recv_data->mac_addr));
                zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(data->device_type), "Firmware update fail. Incorrect bin file.");
                heap_caps_free(mac);
            }
            break;
        case ZHPT_UPDATE_SUCCESS:
            esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_success", 0, 2, true);
            if (gateway_config->syslog_is_enable == true)
            {
                char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                memset(mac, 0, 18);
                sprintf(mac, "" MAC_STR "", MAC2STR(recv_data->mac_addr));
                zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(data->device_type), "Firmware update success.");
                heap_caps_free(mac);
            }
            break;
        case ZHPT_ERROR:
            if (gateway_config->syslog_is_enable == true)
            {
                char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                memset(mac, 0, 18);
                sprintf(mac, "" MAC_STR "", MAC2STR(recv_data->mac_addr));
                zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(data->device_type), data->payload_data.status_message.error_message.message);
                heap_caps_free(mac);
            }
            break;
        default:
            break;
        }
        heap_caps_free(topic);
#ifdef CONFIG_NETWORK_TYPE_DIRECT
    ZH_ESPNOW_EVENT_HANDLER_EXIT:
        heap_caps_free(recv_data->data);
        break;
    case ZH_ESPNOW_ON_SEND_EVENT:
        break;
#else
    ZH_NETWORK_EVENT_HANDLER_EXIT:
        heap_caps_free(recv_data->data);
        break;
    case ZH_NETWORK_ON_SEND_EVENT:
        break;
#endif
    default:
        break;
    }
}

void zh_mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gateway_config_t *gateway_config = arg;
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        if (gateway_config->mqtt_is_connected == false)
        {
            if (gateway_config->syslog_is_enable == true)
            {
                char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                memset(mac, 0, 18);
                sprintf(mac, "" MAC_STR "", MAC2STR(gateway_config->self_mac));
                zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Connected to MQTT.");
                heap_caps_free(mac);
            }
            char *topic_for_subscribe = NULL;
            char *supported_device_type = NULL;
            for (zh_device_type_t i = 1; i <= ZHDT_MAX; ++i)
            {
                supported_device_type = zh_get_device_type_value_name(i);
                topic_for_subscribe = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(supported_device_type) + 4, MALLOC_CAP_8BIT);
                memset(topic_for_subscribe, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(supported_device_type) + 4);
                sprintf(topic_for_subscribe, "%s/%s/#", gateway_config->software_config.mqtt_topic_prefix, supported_device_type);
                esp_mqtt_client_subscribe(gateway_config->mqtt_client, topic_for_subscribe, 2);
                heap_caps_free(topic_for_subscribe);
            }
            zh_gateway_send_mqtt_json_config_message(gateway_config);
            xTaskCreatePinnedToCore(&zh_gateway_send_mqtt_json_attributes_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, gateway_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&gateway_config->gateway_attributes_message_task, tskNO_AFFINITY);
            xTaskCreatePinnedToCore(&zh_gateway_send_mqtt_json_keep_alive_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, gateway_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&gateway_config->gateway_keep_alive_message_task, tskNO_AFFINITY);
            xTaskCreatePinnedToCore(&zh_device_availability_check_task, "NULL", ZH_CHECK_STACK_SIZE, gateway_config, ZH_CHECK_TASK_PRIORITY, (TaskHandle_t *)&gateway_config->device_availability_check_task, tskNO_AFFINITY);
        }
        gateway_config->mqtt_is_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        if (gateway_config->mqtt_is_connected == true)
        {
            if (gateway_config->syslog_is_enable == true)
            {
                char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                memset(mac, 0, 18);
                sprintf(mac, "" MAC_STR "", MAC2STR(gateway_config->self_mac));
                zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Disconnected from MQTT.");
                heap_caps_free(mac);
            }
            vTaskDelete(gateway_config->gateway_attributes_message_task);
            vTaskDelete(gateway_config->gateway_keep_alive_message_task);
            vTaskDelete(gateway_config->device_availability_check_task);
            zh_espnow_data_t data = {0};
            data.device_type = ZHDT_GATEWAY;
            data.payload_type = ZHPT_KEEP_ALIVE;
            data.payload_data.keep_alive_message.online_status = ZH_OFFLINE;
            zh_send_message(NULL, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        }
        gateway_config->mqtt_is_connected = false;
        break;
    case MQTT_EVENT_DATA:
        char incoming_topic[(event->topic_len) + 1];
        memset(incoming_topic, 0, sizeof(incoming_topic));
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
        memset(incoming_payload, 0, sizeof(incoming_payload));
        strncat(incoming_payload, event->data, event->data_len);
        zh_espnow_data_t data = {0};
        data.device_type = ZHDT_GATEWAY;
        switch (incoming_data_device_type)
        {
        case ZHDT_GATEWAY:
            if (incoming_data_payload_type == ZHPT_NONE)
            {
                if (memcmp(gateway_config->self_mac, incoming_data_mac, 6) == 0)
                {
                    if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && gateway_config->software_config.is_ota_server_usage == true)
                    {
                        if (xSemaphoreTake(gateway_config->self_ota_in_progress_mutex, portTICK_PERIOD_MS) == pdTRUE)
                        {
                            xTaskCreatePinnedToCore(&zh_self_ota_update_task, "NULL", ZH_OTA_STACK_SIZE, gateway_config, ZH_OTA_TASK_PRIORITY, NULL, tskNO_AFFINITY);
                            xSemaphoreGive(gateway_config->self_ota_in_progress_mutex);
                        }
                    }
                    else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                    {
                        zh_espnow_data_t data = {0};
                        data.device_type = ZHDT_GATEWAY;
                        data.payload_type = ZHPT_KEEP_ALIVE;
                        data.payload_data.keep_alive_message.online_status = ZH_OFFLINE;
                        zh_send_message(NULL, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
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
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && gateway_config->software_config.is_ota_server_usage == true)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
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
                        data.payload_data.status_message.switch_status_message.status = i;
                        data.payload_type = ZHPT_SET;
                        zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                        break;
                    }
                }
                break;
            case ZHPT_HARDWARE:
                char *extracted_hardware_data = strtok(incoming_payload, ","); // Extract relay gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.relay_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract relay on level value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.relay_on_level = zh_bool_value_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract led gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.led_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract led on level value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.led_on_level = zh_bool_value_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract internal button GPIO number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.int_button_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract internal button on level value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.int_button_on_level = zh_bool_value_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract external button GPIO number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.ext_button_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract external button on level value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.ext_button_on_level = zh_bool_value_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract sensor GPIO number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.sensor_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract sensor type value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.switch_hardware_config_message.sensor_type = zh_sensor_type_check(atoi(extracted_hardware_data));
                data.payload_type = ZHPT_HARDWARE;
                zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            default:
                break;
            }
            break;
        case ZHDT_LED:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && gateway_config->software_config.is_ota_server_usage == true)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
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
                        data.payload_data.status_message.led_status_message.status = i;
                        data.payload_type = ZHPT_SET;
                        zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                        break;
                    }
                }
                break;
            case ZHPT_BRIGHTNESS:
                data.payload_data.status_message.led_status_message.brightness = atoi(incoming_payload);
                data.payload_type = ZHPT_BRIGHTNESS;
                zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_TEMPERATURE:
                data.payload_data.status_message.led_status_message.temperature = atoi(incoming_payload);
                data.payload_type = ZHPT_TEMPERATURE;
                zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_RGB:
                char *extracted_rgb_data = strtok(incoming_payload, ","); // Extract red value.
                if (extracted_rgb_data == NULL)
                {
                    break;
                }
                data.payload_data.status_message.led_status_message.red = atoi(extracted_rgb_data);
                extracted_rgb_data = strtok(NULL, ","); // Extract green value.
                if (extracted_rgb_data == NULL)
                {
                    break;
                }
                data.payload_data.status_message.led_status_message.green = atoi(extracted_rgb_data);
                extracted_rgb_data = strtok(NULL, ","); // Extract blue value.
                if (extracted_rgb_data == NULL)
                {
                    break;
                }
                data.payload_data.status_message.led_status_message.blue = atoi(extracted_rgb_data);
                data.payload_type = ZHPT_RGB;
                zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_HARDWARE:
                char *extracted_hardware_data = strtok(incoming_payload, ","); // Extract led type value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.led_hardware_config_message.led_type = zh_led_type_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract first white gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.led_hardware_config_message.first_white_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract second white gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.led_hardware_config_message.second_white_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract red gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.led_hardware_config_message.red_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract green gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.led_hardware_config_message.green_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract blue gpio number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.led_hardware_config_message.blue_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                data.payload_type = ZHPT_HARDWARE;
                zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            default:
                break;
            }
            break;
        case ZHDT_SENSOR:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && gateway_config->software_config.is_ota_server_usage == true)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else
                {
                    break;
                }
                break;
            case ZHPT_HARDWARE:
                char *extracted_hardware_data = strtok(incoming_payload, ","); // Extract sensor type value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.sensor_hardware_config_message.sensor_type = zh_sensor_type_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract sensor GPIO number 1 value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.sensor_hardware_config_message.sensor_pin_1 = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract sensor GPIO number 2 value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.sensor_hardware_config_message.sensor_pin_2 = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract sensor power control GPIO number value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.sensor_hardware_config_message.power_pin = zh_gpio_number_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract sensor measurement frequency value.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.sensor_hardware_config_message.measurement_frequency = zh_uint16_value_check(atoi(extracted_hardware_data));
                extracted_hardware_data = strtok(NULL, ","); // Extract power mode.
                if (extracted_hardware_data == NULL)
                {
                    break;
                }
                data.payload_data.config_message.sensor_hardware_config_message.battery_power = zh_bool_value_check(atoi(extracted_hardware_data));
                data.payload_type = ZHPT_HARDWARE;
                zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            default:
                break;
            }
            break;
        case ZHDT_BINARY_SENSOR:
            switch (incoming_data_payload_type)
            {
            case ZHPT_NONE:
                if (strncmp(incoming_payload, "update", strlen(incoming_payload) + 1) == 0 && gateway_config->software_config.is_ota_server_usage == true)
                {
                    data.payload_type = ZHPT_UPDATE;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                }
                else if (strncmp(incoming_payload, "restart", strlen(incoming_payload) + 1) == 0)
                {
                    data.payload_type = ZHPT_RESTART;
                    zh_send_message(incoming_data_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
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

void zh_self_ota_update_task(void *pvParameter)
{
    gateway_config_t *gateway_config = pvParameter;
    char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
    memset(mac, 0, 18);
    sprintf(mac, "" MAC_STR "", MAC2STR(gateway_config->self_mac));
    xSemaphoreTake(gateway_config->self_ota_in_progress_mutex, portMAX_DELAY);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(ZHDT_GATEWAY)) + 20, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(ZHDT_GATEWAY)) + 20);
    sprintf(topic, "%s/%s/" MAC_STR, gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(ZHDT_GATEWAY), MAC2STR(gateway_config->self_mac));
    char self_ota_write_data[1025] = {0};
    esp_ota_handle_t update_handle = {0};
    const esp_partition_t *update_partition = NULL;
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_begin", 0, 2, true);
    if (gateway_config->syslog_is_enable == true)
    {
        zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Firmware update begin.");
    }
    const esp_app_desc_t *app_info = esp_app_get_description();
    char *app_name = (char *)heap_caps_malloc(strlen(gateway_config->software_config.firmware_upgrade_url) + strlen(app_info->project_name) + 6, MALLOC_CAP_8BIT);
    memset(app_name, 0, strlen(gateway_config->software_config.firmware_upgrade_url) + strlen(app_info->project_name) + 6);
    sprintf(app_name, "%s/%s.bin", gateway_config->software_config.firmware_upgrade_url, app_info->project_name);
    esp_http_client_config_t config = {
        .url = app_name,
        .cert_pem = (char *)server_certificate_pem_start,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t https_client = esp_http_client_init(&config);
    heap_caps_free(app_name);
    esp_http_client_open(https_client, 0);
    esp_http_client_fetch_headers(https_client);
    update_partition = esp_ota_get_next_update_partition(NULL);
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_progress", 0, 2, true);
    if (gateway_config->syslog_is_enable == true)
    {
        zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Firmware update progress.");
    }
    esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    for (;;)
    {
        int data_read_size = esp_http_client_read(https_client, self_ota_write_data, 1024);
        if (data_read_size < 0)
        {
            esp_http_client_close(https_client);
            esp_http_client_cleanup(https_client);
            esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_error_data_size", 0, 2, true);
            if (gateway_config->syslog_is_enable == true)
            {
                zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Firmware update error. Incorrect size of read data.");
            }
            heap_caps_free(topic);
            heap_caps_free(mac);
            xSemaphoreGive(gateway_config->self_ota_in_progress_mutex);
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
        esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_fail", 0, 2, true);
        if (gateway_config->syslog_is_enable == true)
        {
            zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Firmware update fail. Incorrect bin file.");
        }
        heap_caps_free(topic);
        heap_caps_free(mac);
        xSemaphoreGive(gateway_config->self_ota_in_progress_mutex);
        vTaskDelete(NULL);
    }
    esp_ota_set_boot_partition(update_partition);
    esp_http_client_close(https_client);
    esp_http_client_cleanup(https_client);
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_success", 0, 2, true);
    if (gateway_config->syslog_is_enable == true)
    {
        zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Firmware update success.");
    }
    heap_caps_free(topic);
    heap_caps_free(mac);
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    data.payload_type = ZHPT_KEEP_ALIVE;
    data.payload_data.keep_alive_message.online_status = ZH_OFFLINE;
    zh_send_message(NULL, (uint8_t *)&data, sizeof(zh_espnow_data_t));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
}

void zh_espnow_ota_update_task(void *pvParameter)
{
    gateway_config_t *gateway_config = pvParameter;
    char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
    memset(mac, 0, 18);
    sprintf(mac, "" MAC_STR "", MAC2STR(gateway_config->espnow_ota_data.mac_addr));
    // heap_caps_free(mac);
    xSemaphoreTake(gateway_config->espnow_ota_in_progress_mutex, portMAX_DELAY);
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type)) + 20, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type)) + 20);
    sprintf(topic, "%s/%s/" MAC_STR, gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), MAC2STR(gateway_config->espnow_ota_data.mac_addr));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_begin", 0, 2, true);
    if (gateway_config->syslog_is_enable == true)
    {
        zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), "Firmware update begin.");
    }
    char espnow_ota_write_data[sizeof(data.payload_data.ota_message.espnow_ota_message.data) + 1] = {0};
    char *app_name = (char *)heap_caps_malloc(strlen(gateway_config->software_config.firmware_upgrade_url) + strlen(gateway_config->espnow_ota_data.app_name) + 6, MALLOC_CAP_8BIT);
    memset(app_name, 0, strlen(gateway_config->software_config.firmware_upgrade_url) + strlen(gateway_config->espnow_ota_data.app_name) + 6);
    sprintf(app_name, "%s/%s.bin", gateway_config->software_config.firmware_upgrade_url, gateway_config->espnow_ota_data.app_name);
    esp_http_client_config_t config = {
        .url = app_name,
        .cert_pem = (char *)server_certificate_pem_start,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t https_client = esp_http_client_init(&config);
    heap_caps_free(app_name);
    esp_http_client_open(https_client, 0);
    esp_http_client_fetch_headers(https_client);
    data.payload_type = ZHPT_UPDATE_BEGIN;
    zh_send_message(gateway_config->espnow_ota_data.mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
    if (xSemaphoreTake(gateway_config->espnow_ota_data_semaphore, 30000 / portTICK_PERIOD_MS) != pdTRUE)
    {
        esp_http_client_close(https_client);
        esp_http_client_cleanup(https_client);
        data.payload_type = ZHPT_UPDATE_ERROR;
        zh_send_message(gateway_config->espnow_ota_data.mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_error_begin_timeout", 0, 2, true);
        if (gateway_config->syslog_is_enable == true)
        {
            zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), "Firmware update error. Timeout exceed.");
        }
        heap_caps_free(mac);
        heap_caps_free(topic);
        xSemaphoreGive(gateway_config->espnow_ota_in_progress_mutex);
        vTaskDelete(NULL);
    }
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_progress", 0, 2, true);
    if (gateway_config->syslog_is_enable == true)
    {
        zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), "Firmware update progress.");
    }
    for (;;)
    {
        int data_read_size = esp_http_client_read(https_client, espnow_ota_write_data, sizeof(data.payload_data.ota_message.espnow_ota_message.data));
        if (data_read_size < 0)
        {
            esp_http_client_close(https_client);
            esp_http_client_cleanup(https_client);
            esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_error_data_size", 0, 2, true);
            if (gateway_config->syslog_is_enable == true)
            {
                zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), "Firmware update error. Incorrect size of read data.");
            }
            heap_caps_free(mac);
            heap_caps_free(topic);
            xSemaphoreGive(gateway_config->espnow_ota_in_progress_mutex);
            vTaskDelete(NULL);
        }
        else if (data_read_size > 0)
        {
            data.payload_data.ota_message.espnow_ota_message.data_len = data_read_size;
            ++data.payload_data.ota_message.espnow_ota_message.part;
            memcpy(data.payload_data.ota_message.espnow_ota_message.data, espnow_ota_write_data, data_read_size);
            data.payload_type = ZHPT_UPDATE_PROGRESS;
            zh_send_message(gateway_config->espnow_ota_data.mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
            if (xSemaphoreTake(gateway_config->espnow_ota_data_semaphore, 15000 / portTICK_PERIOD_MS) != pdTRUE)
            {
                esp_http_client_close(https_client);
                esp_http_client_cleanup(https_client);
                data.payload_type = ZHPT_UPDATE_ERROR;
                zh_send_message(gateway_config->espnow_ota_data.mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_error_progress_timeout", 0, 2, true);
                if (gateway_config->syslog_is_enable == true)
                {
                    zh_syslog_send(ZH_USER, ZH_ERR, mac, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), "Firmware update error. Timeout exceed.");
                }
                heap_caps_free(mac);
                heap_caps_free(topic);
                xSemaphoreGive(gateway_config->espnow_ota_in_progress_mutex);
                vTaskDelete(NULL);
            }
        }
        else
        {
            esp_http_client_close(https_client);
            esp_http_client_cleanup(https_client);
            data.payload_type = ZHPT_UPDATE_END;
            zh_send_message(gateway_config->espnow_ota_data.mac_addr, (uint8_t *)&data, sizeof(zh_espnow_data_t));
            esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "update_end", 0, 2, true);
            if (gateway_config->syslog_is_enable == true)
            {
                zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(gateway_config->espnow_ota_data.device_type), "Firmware update end.");
            }
            heap_caps_free(mac);
            heap_caps_free(topic);
            xSemaphoreGive(gateway_config->espnow_ota_in_progress_mutex);
            vTaskDelete(NULL);
        }
    }
}

void zh_send_espnow_current_time_task(void *pvParameter)
{
    gateway_config_t *gateway_config = pvParameter;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    if (gateway_config->syslog_is_enable == true)
    {
        char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
        memset(mac, 0, 18);
        sprintf(mac, "" MAC_STR "", MAC2STR(gateway_config->self_mac));
        zh_syslog_send(ZH_USER, ZH_INFO, mac, zh_get_device_type_value_name(ZHDT_GATEWAY), "Connected to NTP.");
        heap_caps_free(mac);
    }
    time_t now;
    setenv("TZ", gateway_config->software_config.ntp_time_zone, 1);
    tzset();
    for (;;)
    {
        time(&now);
        // To Do.
        vTaskDelay(86400000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_device_availability_check_task(void *pvParameter)
{
    gateway_config_t *gateway_config = pvParameter;
    for (;;)
    {
        xSemaphoreTake(gateway_config->device_check_in_progress_mutex, portMAX_DELAY);
        for (uint16_t i = 0; i < zh_vector_get_size(&gateway_config->available_device_vector); ++i)
        {
        CHECK:
            available_device_t *available_device = zh_vector_get_item(&gateway_config->available_device_vector, i);
            if (available_device == NULL)
            {
                break;
            }
            if (esp_timer_get_time() / 1000000 > available_device->time + (available_device->frequency * 3))
            {
                char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(available_device->device_type)) + 27, MALLOC_CAP_8BIT);
                memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(available_device->device_type)) + 27);
                sprintf(topic, "%s/%s/" MAC_STR "/status", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(available_device->device_type), MAC2STR(available_device->mac_addr));
                esp_mqtt_client_publish(gateway_config->mqtt_client, topic, "offline", 0, 2, true);
                heap_caps_free(topic);
                if (gateway_config->syslog_is_enable == true)
                {
                    char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
                    memset(mac, 0, 18);
                    sprintf(mac, "" MAC_STR "", MAC2STR(available_device->mac_addr));
                    zh_syslog_send(ZH_USER, ZH_WARNING, mac, zh_get_device_type_value_name(available_device->device_type), "Disconnected from gateway.");
                    heap_caps_free(mac);
                }
                zh_vector_delete_item(&gateway_config->available_device_vector, i);
                goto CHECK; // Since the vector is shifted after item deletion, the item needs to be re-checked.
            }
        }
        xSemaphoreGive(gateway_config->device_check_in_progress_mutex);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

uint8_t zh_gpio_number_check(int gpio)
{
    return (gpio >= 0 && gpio <= ZH_MAX_GPIO_NUMBERS) ? gpio : ZH_NOT_USED;
}

bool zh_bool_value_check(int value)
{
    return (value == 0) ? false : true;
}

uint8_t zh_sensor_type_check(int type)
{
    return (type > HAST_NONE && type < HAST_MAX) ? type : HAST_NONE;
}

uint8_t zh_led_type_check(int type)
{
    return (type > HALT_NONE && type < HALT_MAX) ? type : HALT_NONE;
}

uint16_t zh_uint16_value_check(int value)
{
    return (value >= 0 && value <= 65536) ? value : 60;
}

void zh_gateway_send_mqtt_json_attributes_message_task(void *pvParameter)
{
    gateway_config_t *gateway_config = pvParameter;
    const esp_app_desc_t *app_info = esp_app_get_description();
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    data.payload_type = ZHPT_ATTRIBUTES;
    data.payload_data.attributes_message.chip_type = ZH_CHIP_TYPE;
    strcpy(data.payload_data.attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    data.payload_data.attributes_message.cpu_frequency = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    data.payload_data.attributes_message.reset_reason = (uint8_t)esp_reset_reason();
    strcpy(data.payload_data.attributes_message.app_name, app_info->project_name);
    strcpy(data.payload_data.attributes_message.app_version, app_info->version);
    for (;;)
    {
        data.payload_data.attributes_message.heap_size = esp_get_free_heap_size();
        data.payload_data.attributes_message.min_heap_size = esp_get_minimum_free_heap_size();
        data.payload_data.attributes_message.uptime = esp_timer_get_time() / 1000000;
        zh_espnow_send_mqtt_json_attributes_message(&data, gateway_config->self_mac, gateway_config);
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_gateway_send_mqtt_json_config_message(const gateway_config_t *gateway_config)
{
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    data.payload_type = ZHPT_CONFIG;
    data.payload_data.config_message.binary_sensor_config_message.unique_id = 1;
    data.payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class = HABSDC_CONNECTIVITY;
    data.payload_data.config_message.binary_sensor_config_message.payload_on = HAONOFT_CONNECT;
    data.payload_data.config_message.binary_sensor_config_message.payload_off = HAONOFT_NONE;
    data.payload_data.config_message.binary_sensor_config_message.expire_after = 30;
    data.payload_data.config_message.binary_sensor_config_message.enabled_by_default = true;
    data.payload_data.config_message.binary_sensor_config_message.force_update = true;
    data.payload_data.config_message.binary_sensor_config_message.qos = 2;
    data.payload_data.config_message.binary_sensor_config_message.retain = true;
    zh_espnow_binary_sensor_send_mqtt_json_config_message(&data, gateway_config->self_mac, gateway_config);
}

void zh_gateway_send_mqtt_json_keep_alive_message_task(void *pvParameter)
{
    gateway_config_t *gateway_config = pvParameter;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_GATEWAY;
    data.payload_data.keep_alive_message.online_status = ZH_ONLINE;
    data.payload_data.status_message.binary_sensor_status_message.sensor_type = HAST_GATEWAY;
    data.payload_data.status_message.binary_sensor_status_message.connect = HAONOFT_CONNECT;
    for (;;)
    {
        data.payload_type = ZHPT_KEEP_ALIVE;
        zh_send_message(NULL, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        data.payload_type = ZHPT_STATE;
        zh_espnow_binary_sensor_send_mqtt_json_status_message(&data, gateway_config->self_mac, gateway_config);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_espnow_send_mqtt_json_attributes_message(zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    uint32_t secs = device_data->payload_data.attributes_message.uptime;
    uint32_t mins = secs / 60;
    uint32_t hours = mins / 60;
    uint32_t days = hours / 24;
    char *mac = (char *)heap_caps_malloc(18, MALLOC_CAP_8BIT);
    memset(mac, 0, 18);
    sprintf(mac, "" MAC_STR "", MAC2STR(device_mac));
    char *uptime = (char *)heap_caps_malloc(44, MALLOC_CAP_8BIT);
    memset(uptime, 0, 44);
    sprintf(uptime, "Days:%lu Hours:%lu Mins:%lu", days, hours - (days * 24), mins - (hours * 60));
    char *cpu_frequency = heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(cpu_frequency, 0, 4);
    sprintf(cpu_frequency, "%d", device_data->payload_data.attributes_message.cpu_frequency);
    char *heap_size = heap_caps_malloc(7, MALLOC_CAP_8BIT);
    memset(heap_size, 0, 7);
    sprintf(heap_size, "%lu", device_data->payload_data.attributes_message.heap_size);
    char *min_heap_size = heap_caps_malloc(7, MALLOC_CAP_8BIT);
    memset(min_heap_size, 0, 7);
    sprintf(min_heap_size, "%lu", device_data->payload_data.attributes_message.min_heap_size);
    char *reset_reason = heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(reset_reason, 0, 4);
    sprintf(reset_reason, "%d", device_data->payload_data.attributes_message.reset_reason);
    zh_json_t json = {0};
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
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(topic, "%s/%s/" MAC_STR "/attributes", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(mac);
    heap_caps_free(uptime);
    heap_caps_free(cpu_frequency);
    heap_caps_free(heap_size);
    heap_caps_free(min_heap_size);
    heap_caps_free(reset_reason);
    heap_caps_free(topic);
}

void zh_espnow_send_mqtt_json_keep_alive_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *status = (device_data->payload_data.keep_alive_message.online_status == ZH_ONLINE) ? "online" : "offline";
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(topic, "%s/%s/" MAC_STR "/status", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, status, 0, 2, true);
    heap_caps_free(topic);
}

void zh_espnow_switch_send_mqtt_json_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *name = (char *)heap_caps_malloc(strlen(zh_get_device_type_value_name(device_data->device_type)) + 19, MALLOC_CAP_8BIT);
    memset(name, 0, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)heap_caps_malloc(22, MALLOC_CAP_8BIT);
    memset(unique_id, 0, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.switch_config_message.unique_id);
    char *state_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26, MALLOC_CAP_8BIT);
    memset(state_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *availability_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27, MALLOC_CAP_8BIT);
    memset(availability_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(availability_topic, "%s/%s/" MAC_STR "/status", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *command_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24, MALLOC_CAP_8BIT);
    memset(command_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24);
    sprintf(command_topic, "%s/%s/" MAC_STR "/set", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *json_attributes_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(json_attributes_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(qos, 0, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.switch_config_message.qos);
    zh_json_t json = {0};
    char buffer[1024] = {0};
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
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 16, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 16);
    sprintf(topic, "%s/switch/%s/config", gateway_config->software_config.mqtt_topic_prefix, unique_id);
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(name);
    heap_caps_free(unique_id);
    heap_caps_free(state_topic);
    heap_caps_free(availability_topic);
    heap_caps_free(command_topic);
    heap_caps_free(json_attributes_topic);
    heap_caps_free(qos);
    heap_caps_free(topic);
}

void zh_espnow_switch_send_mqtt_json_hardware_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *relay_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(relay_pin, 0, 4);
    char *led_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(led_pin, 0, 4);
    char *int_button_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(int_button_pin, 0, 4);
    char *ext_button_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(ext_button_pin, 0, 4);
    char *sensor_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(sensor_pin, 0, 4);
    zh_json_t json = {0};
    char buffer[512] = {0};
    zh_json_init(&json);
    if (device_data->payload_data.config_message.switch_hardware_config_message.relay_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Relay GPIO number", "Not used");
        zh_json_add(&json, "Relay ON level", "Not used");
    }
    else
    {
        sprintf(relay_pin, "%d", device_data->payload_data.config_message.switch_hardware_config_message.relay_pin);
        zh_json_add(&json, "Relay GPIO number", relay_pin);
        zh_json_add(&json, "Relay ON level", (device_data->payload_data.config_message.switch_hardware_config_message.relay_on_level == ZH_LOW) ? "Low" : "High");
    }
    if (device_data->payload_data.config_message.switch_hardware_config_message.led_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Led GPIO number", "Not used");
        zh_json_add(&json, "Led ON level", "Not used");
    }
    else
    {
        sprintf(led_pin, "%d", device_data->payload_data.config_message.switch_hardware_config_message.led_pin);
        zh_json_add(&json, "Led GPIO number", led_pin);
        zh_json_add(&json, "Led ON level", (device_data->payload_data.config_message.switch_hardware_config_message.led_on_level == ZH_LOW) ? "Low" : "High");
    }
    if (device_data->payload_data.config_message.switch_hardware_config_message.int_button_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Internal button GPIO number", "Not used");
        zh_json_add(&json, "Internal button trigger level", "Not used");
    }
    else
    {
        sprintf(int_button_pin, "%d", device_data->payload_data.config_message.switch_hardware_config_message.int_button_pin);
        zh_json_add(&json, "Internal button GPIO number", int_button_pin);
        zh_json_add(&json, "Internal button trigger level", (device_data->payload_data.config_message.switch_hardware_config_message.int_button_on_level == ZH_LOW) ? "Low" : "High");
    }
    if (device_data->payload_data.config_message.switch_hardware_config_message.ext_button_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "External button GPIO number", "Not used");
        zh_json_add(&json, "External button trigger level", "Not used");
    }
    else
    {
        sprintf(ext_button_pin, "%d", device_data->payload_data.config_message.switch_hardware_config_message.ext_button_pin);
        zh_json_add(&json, "External button GPIO number", ext_button_pin);
        zh_json_add(&json, "External button trigger level", (device_data->payload_data.config_message.switch_hardware_config_message.ext_button_on_level == ZH_LOW) ? "Low" : "High");
    }

    if (device_data->payload_data.config_message.switch_hardware_config_message.sensor_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Sensor GPIO number", "Not used");
        zh_json_add(&json, "Sensor type", "Not used");
    }
    else
    {
        sprintf(sensor_pin, "%d", device_data->payload_data.config_message.switch_hardware_config_message.sensor_pin);
        zh_json_add(&json, "Sensor GPIO number", sensor_pin);
        zh_json_add(&json, "Sensor type", zh_get_sensor_type_value_name(device_data->payload_data.config_message.switch_hardware_config_message.sensor_type));
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(topic, "%s/%s/" MAC_STR "/config", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(relay_pin);
    heap_caps_free(led_pin);
    heap_caps_free(int_button_pin);
    heap_caps_free(ext_button_pin);
    heap_caps_free(sensor_pin);
    heap_caps_free(topic);
}

void zh_espnow_switch_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    zh_json_t json = {0};
    char buffer[128] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "state", zh_get_on_off_type_value_name(device_data->payload_data.status_message.switch_status_message.status));
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(topic);
}

void zh_espnow_led_send_mqtt_json_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *name = (char *)heap_caps_malloc(strlen(zh_get_device_type_value_name(device_data->device_type)) + 19, MALLOC_CAP_8BIT);
    memset(name, 0, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)heap_caps_malloc(22, MALLOC_CAP_8BIT);
    memset(unique_id, 0, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.led_config_message.unique_id);
    char *state_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26, MALLOC_CAP_8BIT);
    memset(state_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *availability_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27, MALLOC_CAP_8BIT);
    memset(availability_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(availability_topic, "%s/%s/" MAC_STR "/status", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *command_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24, MALLOC_CAP_8BIT);
    memset(command_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24);
    sprintf(command_topic, "%s/%s/" MAC_STR "/set", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *json_attributes_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(json_attributes_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(qos, 0, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.led_config_message.qos);
    char *brightness_command_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(brightness_command_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(brightness_command_topic, "%s/%s/" MAC_STR "/brightness", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *color_temp_command_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 32, MALLOC_CAP_8BIT);
    memset(color_temp_command_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 32);
    sprintf(color_temp_command_topic, "%s/%s/" MAC_STR "/temperature", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *rgb_command_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24, MALLOC_CAP_8BIT);
    memset(rgb_command_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 24);
    sprintf(rgb_command_topic, "%s/%s/" MAC_STR "/rgb", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    zh_json_t json = {0};
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
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 15, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 15);
    sprintf(topic, "%s/light/%s/config", gateway_config->software_config.mqtt_topic_prefix, unique_id);
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(name);
    heap_caps_free(unique_id);
    heap_caps_free(state_topic);
    heap_caps_free(availability_topic);
    heap_caps_free(command_topic);
    heap_caps_free(json_attributes_topic);
    heap_caps_free(qos);
    heap_caps_free(brightness_command_topic);
    heap_caps_free(color_temp_command_topic);
    heap_caps_free(rgb_command_topic);
    heap_caps_free(topic);
}

void zh_espnow_led_send_mqtt_json_hardware_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *first_white_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(first_white_pin, 0, 4);
    char *second_white_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(second_white_pin, 0, 4);
    char *red_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(red_pin, 0, 4);
    char *green_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(green_pin, 0, 4);
    char *blue_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(blue_pin, 0, 4);
    zh_json_t json = {0};
    char buffer[512] = {0};
    zh_json_init(&json);
    if (device_data->payload_data.config_message.led_hardware_config_message.first_white_pin == ZH_NOT_USED && device_data->payload_data.config_message.led_hardware_config_message.red_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Led type", "Not used");
    }
    else
    {
        switch (device_data->payload_data.config_message.led_hardware_config_message.led_type)
        {
        case HALT_W:
            zh_json_add(&json, "Led type", "W");
            break;
        case HALT_WW:
            zh_json_add(&json, "Led type", "WW");
            break;
        case HALT_RGB:
            zh_json_add(&json, "Led type", "RGB");
            break;
        case HALT_RGBW:
            zh_json_add(&json, "Led type", "RGBW");
            break;
        case HALT_RGBWW:
            zh_json_add(&json, "Led type", "RGBWW");
            break;
        default:
            zh_json_add(&json, "Led type", "Not used");
            break;
        }
    }
    if (device_data->payload_data.config_message.led_hardware_config_message.first_white_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "First white GPIO number", "Not used");
    }
    else
    {
        sprintf(first_white_pin, "%d", device_data->payload_data.config_message.led_hardware_config_message.first_white_pin);
        zh_json_add(&json, "First white GPIO number", first_white_pin);
    }
    if (device_data->payload_data.config_message.led_hardware_config_message.second_white_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Second white GPIO number", "Not used");
    }
    else
    {
        sprintf(second_white_pin, "%d", device_data->payload_data.config_message.led_hardware_config_message.second_white_pin);
        zh_json_add(&json, "Second white GPIO number", second_white_pin);
    }
    if (device_data->payload_data.config_message.led_hardware_config_message.red_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Red GPIO number", "Not used");
    }
    else
    {
        sprintf(red_pin, "%d", device_data->payload_data.config_message.led_hardware_config_message.red_pin);
        zh_json_add(&json, "Red GPIO number", red_pin);
    }
    if (device_data->payload_data.config_message.led_hardware_config_message.green_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Green GPIO number", "Not used");
    }
    else
    {
        sprintf(green_pin, "%d", device_data->payload_data.config_message.led_hardware_config_message.green_pin);
        zh_json_add(&json, "Green GPIO number", green_pin);
    }
    if (device_data->payload_data.config_message.led_hardware_config_message.blue_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Blue GPIO number", "Not used");
    }
    else
    {
        sprintf(blue_pin, "%d", device_data->payload_data.config_message.led_hardware_config_message.blue_pin);
        zh_json_add(&json, "Blue GPIO number", blue_pin);
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(topic, "%s/%s/" MAC_STR "/config", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(first_white_pin);
    heap_caps_free(second_white_pin);
    heap_caps_free(red_pin);
    heap_caps_free(green_pin);
    heap_caps_free(blue_pin);
    heap_caps_free(topic);
}

void zh_espnow_led_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *brightness = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(brightness, 0, 4);
    sprintf(brightness, "%d", device_data->payload_data.status_message.led_status_message.brightness);
    char *temperature = (char *)heap_caps_malloc(6, MALLOC_CAP_8BIT);
    memset(temperature, 0, 6);
    sprintf(temperature, "%d", device_data->payload_data.status_message.led_status_message.temperature);
    char *rgb = (char *)heap_caps_malloc(12, MALLOC_CAP_8BIT);
    memset(rgb, 0, 12);
    sprintf(rgb, "%d,%d,%d", device_data->payload_data.status_message.led_status_message.red, device_data->payload_data.status_message.led_status_message.green, device_data->payload_data.status_message.led_status_message.blue);
    zh_json_t json = {0};
    char buffer[128] = {0};
    zh_json_init(&json);
    zh_json_add(&json, "state", zh_get_on_off_type_value_name(device_data->payload_data.status_message.led_status_message.status));
    zh_json_add(&json, "brightness", brightness);
    zh_json_add(&json, "temperature", temperature);
    zh_json_add(&json, "rgb", rgb);
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(topic);
}

void zh_espnow_sensor_send_mqtt_json_config_message(zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *name = (char *)heap_caps_malloc(strlen(zh_get_device_type_value_name(device_data->device_type)) + 19, MALLOC_CAP_8BIT);
    memset(name, 0, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)heap_caps_malloc(22, MALLOC_CAP_8BIT);
    memset(unique_id, 0, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.sensor_config_message.unique_id);
    char *state_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26, MALLOC_CAP_8BIT);
    memset(state_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *value_template = (char *)heap_caps_malloc(strlen(zh_get_sensor_device_class_value_name(device_data->payload_data.config_message.sensor_config_message.sensor_device_class)) + 18, MALLOC_CAP_8BIT);
    memset(value_template, 0, strlen(zh_get_sensor_device_class_value_name(device_data->payload_data.config_message.sensor_config_message.sensor_device_class)) + 18);
    sprintf(value_template, "{{ value_json.%s }}", zh_get_sensor_device_class_value_name(device_data->payload_data.config_message.sensor_config_message.sensor_device_class));
    char *json_attributes_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(json_attributes_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(qos, 0, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.sensor_config_message.qos);
    char *expire_after = (char *)heap_caps_malloc(6, MALLOC_CAP_8BIT);
    memset(expire_after, 0, 6);
    sprintf(expire_after, "%d", device_data->payload_data.config_message.sensor_config_message.expire_after);
    char *suggested_display_precision = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(suggested_display_precision, 0, 4);
    sprintf(suggested_display_precision, "%d", device_data->payload_data.config_message.sensor_config_message.suggested_display_precision);
    zh_json_t json = {0};
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
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 16, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 16);
    sprintf(topic, "%s/sensor/%s/config", gateway_config->software_config.mqtt_topic_prefix, unique_id);
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(name);
    heap_caps_free(unique_id);
    heap_caps_free(state_topic);
    heap_caps_free(value_template);
    heap_caps_free(json_attributes_topic);
    heap_caps_free(qos);
    heap_caps_free(expire_after);
    heap_caps_free(suggested_display_precision);
    heap_caps_free(topic);
}

void zh_espnow_sensor_send_mqtt_json_hardware_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *sensor_pin_1 = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(sensor_pin_1, 0, 4);
    char *sensor_pin_2 = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(sensor_pin_2, 0, 4);
    char *power_pin = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(power_pin, 0, 4);
    char *measurement_frequency = (char *)heap_caps_malloc(6, MALLOC_CAP_8BIT);
    memset(measurement_frequency, 0, 6);
    zh_json_t json = {0};
    char buffer[512] = {0};
    zh_json_init(&json);
    if (device_data->payload_data.config_message.sensor_hardware_config_message.sensor_pin_1 == ZH_NOT_USED)
    {
        zh_json_add(&json, "Sensor type", "Not used");
        zh_json_add(&json, "Sensor GPIO number 1", "Not used");
    }
    else
    {
        zh_json_add(&json, "Sensor type", zh_get_sensor_type_value_name(device_data->payload_data.config_message.sensor_hardware_config_message.sensor_type));
        sprintf(sensor_pin_1, "%d", device_data->payload_data.config_message.sensor_hardware_config_message.sensor_pin_1);
        zh_json_add(&json, "Sensor GPIO number 1", sensor_pin_1);
    }
    if (device_data->payload_data.config_message.sensor_hardware_config_message.sensor_pin_2 == ZH_NOT_USED)
    {
        zh_json_add(&json, "Sensor GPIO number 2", "Not used");
    }
    else
    {
        if (device_data->payload_data.config_message.sensor_hardware_config_message.sensor_pin_1 == ZH_NOT_USED)
        {
            zh_json_add(&json, "Sensor GPIO number 2", "Not used");
        }
        else
        {
            sprintf(sensor_pin_2, "%d", device_data->payload_data.config_message.sensor_hardware_config_message.sensor_pin_2);
            zh_json_add(&json, "Sensor GPIO number 2", sensor_pin_2);
        }
    }
    if (device_data->payload_data.config_message.sensor_hardware_config_message.power_pin == ZH_NOT_USED)
    {
        zh_json_add(&json, "Sensor power control GPIO number", "Not used");
    }
    else
    {
        sprintf(power_pin, "%d", device_data->payload_data.config_message.sensor_hardware_config_message.power_pin);
        zh_json_add(&json, "Sensor power control GPIO number", power_pin);
    }
    sprintf(measurement_frequency, "%d", device_data->payload_data.config_message.sensor_hardware_config_message.measurement_frequency);
    zh_json_add(&json, "Measurement frequency", measurement_frequency);
    zh_json_add(&json, "Power mode", (device_data->payload_data.config_message.sensor_hardware_config_message.battery_power == true) ? "Battery" : "External");
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 27);
    sprintf(topic, "%s/%s/" MAC_STR "/config", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(sensor_pin_1);
    heap_caps_free(sensor_pin_2);
    heap_caps_free(power_pin);
    heap_caps_free(measurement_frequency);
    heap_caps_free(topic);
}

void zh_espnow_sensor_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *voltage = (char *)heap_caps_malloc(317, MALLOC_CAP_8BIT);
    memset(voltage, 0, 317);
    char *temperature = (char *)heap_caps_malloc(317, MALLOC_CAP_8BIT);
    memset(temperature, 0, 317);
    char *humidity = (char *)heap_caps_malloc(317, MALLOC_CAP_8BIT);
    memset(humidity, 0, 317);
    char *atmospheric_pressure = (char *)heap_caps_malloc(317, MALLOC_CAP_8BIT);
    memset(atmospheric_pressure, 0, 317);
    char *aqi = (char *)heap_caps_malloc(317, MALLOC_CAP_8BIT);
    memset(aqi, 0, 317);
    char *illuminance = (char *)heap_caps_malloc(317, MALLOC_CAP_8BIT);
    memset(illuminance, 0, 317);
    zh_json_t json = {0};
    char buffer[512] = {0};
    zh_json_init(&json);
    sprintf(voltage, "%f", device_data->payload_data.status_message.sensor_status_message.voltage);
    zh_json_add(&json, "voltage", voltage);
    switch (device_data->payload_data.status_message.sensor_status_message.sensor_type)
    {
    case HAST_DS18B20:
        sprintf(temperature, "%f", device_data->payload_data.status_message.sensor_status_message.temperature);
        zh_json_add(&json, "temperature", temperature);
        break;
    case HAST_DHT:
    case HAST_AHT:
    case HAST_SHT:
    case HAST_HTU:
    case HAST_HDC1080:
        sprintf(temperature, "%f", device_data->payload_data.status_message.sensor_status_message.temperature);
        zh_json_add(&json, "temperature", temperature);
        sprintf(humidity, "%f", device_data->payload_data.status_message.sensor_status_message.humidity);
        zh_json_add(&json, "humidity", humidity);
        break;
    case HAST_BH1750:
        sprintf(illuminance, "%f", device_data->payload_data.status_message.sensor_status_message.illuminance);
        zh_json_add(&json, "illuminance", illuminance);
        break;
    case HAST_BMP280:
        sprintf(temperature, "%f", device_data->payload_data.status_message.sensor_status_message.temperature);
        zh_json_add(&json, "temperature", temperature);
        sprintf(atmospheric_pressure, "%f", device_data->payload_data.status_message.sensor_status_message.atmospheric_pressure);
        zh_json_add(&json, "atmospheric_pressure", atmospheric_pressure);
        break;
    case HAST_BME280:
        sprintf(temperature, "%f", device_data->payload_data.status_message.sensor_status_message.temperature);
        zh_json_add(&json, "temperature", temperature);
        sprintf(humidity, "%f", device_data->payload_data.status_message.sensor_status_message.humidity);
        zh_json_add(&json, "humidity", humidity);
        sprintf(atmospheric_pressure, "%f", device_data->payload_data.status_message.sensor_status_message.atmospheric_pressure);
        zh_json_add(&json, "atmospheric_pressure", atmospheric_pressure);
        break;
    case HAST_BME680:
        sprintf(temperature, "%f", device_data->payload_data.status_message.sensor_status_message.temperature);
        zh_json_add(&json, "temperature", temperature);
        sprintf(humidity, "%f", device_data->payload_data.status_message.sensor_status_message.humidity);
        zh_json_add(&json, "humidity", humidity);
        sprintf(atmospheric_pressure, "%f", device_data->payload_data.status_message.sensor_status_message.atmospheric_pressure);
        zh_json_add(&json, "atmospheric_pressure", atmospheric_pressure);
        sprintf(aqi, "%f", device_data->payload_data.status_message.sensor_status_message.aqi);
        zh_json_add(&json, "aqi", aqi);
        break;
    default:
        break;
    }
    zh_json_create(&json, buffer);
    zh_json_free(&json);
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(voltage);
    heap_caps_free(temperature);
    heap_caps_free(humidity);
    heap_caps_free(atmospheric_pressure);
    heap_caps_free(aqi);
    heap_caps_free(illuminance);
    heap_caps_free(topic);
}

void zh_espnow_binary_sensor_send_mqtt_json_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    char *name = (char *)heap_caps_malloc(strlen(zh_get_device_type_value_name(device_data->device_type)) + 19, MALLOC_CAP_8BIT);
    memset(name, 0, strlen(zh_get_device_type_value_name(device_data->device_type)) + 19);
    sprintf(name, "%s " MAC_STR "", zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *unique_id = (char *)heap_caps_malloc(22, MALLOC_CAP_8BIT);
    memset(unique_id, 0, 22);
    sprintf(unique_id, "" MAC_STR "-%d", MAC2STR(device_mac), device_data->payload_data.config_message.binary_sensor_config_message.unique_id);
    char *state_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26, MALLOC_CAP_8BIT);
    memset(state_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 26);
    sprintf(state_topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *value_template = (char *)heap_caps_malloc(strlen(zh_get_binary_sensor_device_class_value_name(device_data->payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class)) + 18, MALLOC_CAP_8BIT);
    memset(value_template, 0, strlen(zh_get_binary_sensor_device_class_value_name(device_data->payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class)) + 18);
    sprintf(value_template, "{{ value_json.%s }}", zh_get_binary_sensor_device_class_value_name(device_data->payload_data.config_message.binary_sensor_config_message.binary_sensor_device_class));
    char *json_attributes_topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(json_attributes_topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(json_attributes_topic, "%s/%s/" MAC_STR "/attributes", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    char *qos = (char *)heap_caps_malloc(4, MALLOC_CAP_8BIT);
    memset(qos, 0, 4);
    sprintf(qos, "%d", device_data->payload_data.config_message.binary_sensor_config_message.qos);
    char *expire_after = (char *)heap_caps_malloc(6, MALLOC_CAP_8BIT);
    memset(expire_after, 0, 6);
    sprintf(expire_after, "%d", device_data->payload_data.config_message.binary_sensor_config_message.expire_after);
    char *off_delay = (char *)heap_caps_malloc(6, MALLOC_CAP_8BIT);
    memset(off_delay, 0, 6);
    sprintf(off_delay, "%d", device_data->payload_data.config_message.binary_sensor_config_message.off_delay);
    zh_json_t json = {0};
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
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 23, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(unique_id) + 23);
    sprintf(topic, "%s/binary_sensor/%s/config", gateway_config->software_config.mqtt_topic_prefix, unique_id);
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(name);
    heap_caps_free(unique_id);
    heap_caps_free(state_topic);
    heap_caps_free(value_template);
    heap_caps_free(json_attributes_topic);
    heap_caps_free(qos);
    heap_caps_free(expire_after);
    heap_caps_free(off_delay);
    heap_caps_free(topic);
}

void zh_espnow_binary_sensor_send_mqtt_json_hardware_config_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
}

void zh_espnow_binary_sensor_send_mqtt_json_status_message(const zh_espnow_data_t *device_data, const uint8_t *device_mac, const gateway_config_t *gateway_config)
{
    zh_json_t json = {0};
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
    char *topic = (char *)heap_caps_malloc(strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31, MALLOC_CAP_8BIT);
    memset(topic, 0, strlen(gateway_config->software_config.mqtt_topic_prefix) + strlen(zh_get_device_type_value_name(device_data->device_type)) + 31);
    sprintf(topic, "%s/%s/" MAC_STR "/state", gateway_config->software_config.mqtt_topic_prefix, zh_get_device_type_value_name(device_data->device_type), MAC2STR(device_mac));
    esp_mqtt_client_publish(gateway_config->mqtt_client, topic, buffer, 0, 2, true);
    heap_caps_free(topic);
}