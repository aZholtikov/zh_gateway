/**
 * @file
 * The main code of the zh_espnow component.
 *
 */

#include "zh_espnow.h"

/// \cond
#define DATA_SEND_SUCCESS BIT0
#define DATA_SEND_FAIL BIT1
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
/// \endcond

static void _send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
#if defined CONFIG_IDF_TARGET_ESP8266 || ESP_IDF_VERSION_MAJOR == 4
static void _recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len);
#else
static void _recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);
#endif
static void _processing(void *pvParameter);

static const char *TAG = "zh_espnow";

static EventGroupHandle_t _event_group_handle = {0};
static QueueHandle_t _queue_handle = {0};
static TaskHandle_t _processing_task_handle = {0};
static zh_espnow_init_config_t _init_config = {0};
static bool _is_initialized = false;
static uint8_t _attempts = 0;

/// \cond
typedef struct
{
    enum
    {
        ON_RECV,
        TO_SEND,
    } id;
    struct
    {
        uint8_t mac_addr[6];
        uint8_t *payload;
        uint8_t payload_len;
    } data;
} _queue_t;

ESP_EVENT_DEFINE_BASE(ZH_ESPNOW);
/// \endcond

esp_err_t zh_espnow_init(const zh_espnow_init_config_t *config)
{
    ESP_LOGI(TAG, "ESP-NOW initialization begin.");
    if (config == NULL)
    {
        ESP_LOGE(TAG, "ESP-NOW initialization fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    _init_config = *config;
    if (_init_config.wifi_channel < 1 || _init_config.wifi_channel > 14)
    {
        ESP_LOGE(TAG, "ESP-NOW initialization fail. WiFi channel.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = esp_wifi_set_channel(_init_config.wifi_channel, WIFI_SECOND_CHAN_NONE);
    if (err == ESP_ERR_WIFI_NOT_INIT || err == ESP_ERR_WIFI_NOT_STARTED)
    {
        ESP_LOGE(TAG, "ESP-NOW initialization fail. WiFi not initialized.");
        return ESP_ERR_WIFI_NOT_INIT;
    }
    else if (err == ESP_FAIL)
    {
        uint8_t prim = 0;
        wifi_second_chan_t sec = 0;
        esp_wifi_get_channel(&prim, &sec);
        if (prim != _init_config.wifi_channel)
        {
            ESP_LOGW(TAG, "ESP-NOW initialization warning. The device is connected to the router. Channel %d will be used for ESP-NOW.", prim);
        }
    }
    _event_group_handle = xEventGroupCreate();
    _queue_handle = xQueueCreate(_init_config.queue_size, sizeof(_queue_t));
    if (esp_now_init() != ESP_OK || esp_now_register_send_cb(_send_cb) != ESP_OK || esp_now_register_recv_cb(_recv_cb) != ESP_OK)
    {
        ESP_LOGE(TAG, "ESP-NOW initialization fail. Internal error.");
        return ESP_FAIL;
    }
    if (xTaskCreatePinnedToCore(&_processing, "NULL", _init_config.stack_size, NULL, _init_config.task_priority, &_processing_task_handle, tskNO_AFFINITY) != pdPASS)
    {
        ESP_LOGE(TAG, "ESP-NOW initialization fail. Internal error.");
        return ESP_FAIL;
    }
    _is_initialized = true;
    ESP_LOGI(TAG, "ESP-NOW initialization success.");
    return ESP_OK;
}

esp_err_t zh_espnow_deinit(void)
{
    ESP_LOGI(TAG, "ESP-NOW deinitialization begin.");
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "ESP-NOW deinitialization fail. ESP-NOW not initialized.");
        return ESP_FAIL;
    }
    vEventGroupDelete(_event_group_handle);
    vQueueDelete(_queue_handle);
    esp_now_unregister_send_cb();
    esp_now_unregister_recv_cb();
    esp_now_deinit();
    vTaskDelete(_processing_task_handle);
    _is_initialized = false;
    ESP_LOGI(TAG, "ESP-NOW deinitialization success.");
    return ESP_OK;
}

esp_err_t zh_espnow_send(const uint8_t *target, const uint8_t *data, const uint8_t data_len)
{
    if (target == NULL)
    {
        ESP_LOGI(TAG, "Adding outgoing ESP-NOW data to MAC FF:FF:FF:FF:FF:FF to queue begin.");
    }
    else
    {
        ESP_LOGI(TAG, "Adding outgoing ESP-NOW data to MAC %02X:%02X:%02X:%02X:%02X:%02X to queue begin.", MAC2STR(target));
    }
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "Adding outgoing ESP-NOW data to queue fail. ESP-NOW not initialized.");
        return ESP_FAIL;
    }
    if (data == NULL || data_len == 0 || data_len > ESP_NOW_MAX_DATA_LEN)
    {
        ESP_LOGE(TAG, "Adding outgoing ESP-NOW data to queue fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    if (uxQueueSpacesAvailable(_queue_handle) < _init_config.queue_size / 10)
    {
        ESP_LOGW(TAG, "Adding outgoing ESP-NOW data to queue fail. Queue is almost full.");
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    _queue_t queue = {0};
    queue.id = TO_SEND;
    if (target == NULL)
    {
        memcpy(queue.data.mac_addr, broadcast, 6);
    }
    else
    {
        memcpy(queue.data.mac_addr, target, 6);
    }
    if (data_len / sizeof(void *) == 0)
    {
        queue.data.payload = heap_caps_malloc(data_len, MALLOC_CAP_32BIT);
    }
    else
    {
        queue.data.payload = heap_caps_malloc(data_len, MALLOC_CAP_8BIT);
    }
    if (queue.data.payload == NULL)
    {
        ESP_LOGE(TAG, "Adding outgoing ESP-NOW data to queue fail. Memory allocation fail or no free memory in the heap.");
        return ESP_ERR_NO_MEM;
    }
    memset(queue.data.payload, 0, data_len);
    memcpy(queue.data.payload, data, data_len);
    queue.data.payload_len = data_len;
    if (target == NULL)
    {
        ESP_LOGI(TAG, "Adding outgoing ESP-NOW data to MAC FF:FF:FF:FF:FF:FF to queue success.");
    }
    else
    {
        ESP_LOGI(TAG, "Adding outgoing ESP-NOW data to MAC %02X:%02X:%02X:%02X:%02X:%02X to queue success.", MAC2STR(target));
    }
    if (xQueueSend(_queue_handle, &queue, portTICK_PERIOD_MS) != pdTRUE)
    {
        ESP_LOGE(TAG, "ESP-NOW message processing task internal error at line %d.", __LINE__);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void _send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        xEventGroupSetBits(_event_group_handle, DATA_SEND_SUCCESS);
    }
    else
    {
        xEventGroupSetBits(_event_group_handle, DATA_SEND_FAIL);
    }
}

#if defined CONFIG_IDF_TARGET_ESP8266 || ESP_IDF_VERSION_MAJOR == 4
static void _recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len)
#else
static void _recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
#endif
{
#if defined CONFIG_IDF_TARGET_ESP8266 || ESP_IDF_VERSION_MAJOR == 4
    ESP_LOGI(TAG, "Adding incoming ESP-NOW data from MAC %02X:%02X:%02X:%02X:%02X:%02X to queue begin.", MAC2STR(mac_addr));
#else
    ESP_LOGI(TAG, "Adding incoming ESP-NOW data from MAC %02X:%02X:%02X:%02X:%02X:%02X to queue begin.", MAC2STR(esp_now_info->src_addr));
#endif
    if (uxQueueSpacesAvailable(_queue_handle) < _init_config.queue_size / 10)
    {
        ESP_LOGW(TAG, "Adding incoming ESP-NOW data to queue fail. Queue is almost full.");
        return;
    }
    _queue_t queue = {0};
    queue.id = ON_RECV;
#if defined CONFIG_IDF_TARGET_ESP8266 || ESP_IDF_VERSION_MAJOR == 4
    memcpy(queue.data.mac_addr, mac_addr, 6);
#else
    memcpy(queue.data.mac_addr, esp_now_info->src_addr, 6);
#endif
    if (data_len / sizeof(void *) == 0)
    {
        queue.data.payload = heap_caps_malloc(data_len, MALLOC_CAP_32BIT);
    }
    else
    {
        queue.data.payload = heap_caps_malloc(data_len, MALLOC_CAP_8BIT);
    }
    if (queue.data.payload == NULL)
    {
        ESP_LOGE(TAG, "Adding incoming ESP-NOW data to queue fail. Memory allocation fail or no free memory in the heap.");
        return;
    }
    memset(queue.data.payload, 0, data_len);
    memcpy(queue.data.payload, data, data_len);
    queue.data.payload_len = data_len;
#if defined CONFIG_IDF_TARGET_ESP8266 || ESP_IDF_VERSION_MAJOR == 4
    ESP_LOGI(TAG, "Adding incoming ESP-NOW data from MAC %02X:%02X:%02X:%02X:%02X:%02X to queue success.", MAC2STR(mac_addr));
#else
    ESP_LOGI(TAG, "Adding incoming ESP-NOW data from MAC %02X:%02X:%02X:%02X:%02X:%02X to queue success.", MAC2STR(esp_now_info->src_addr));
#endif
    if (xQueueSend(_queue_handle, &queue, portTICK_PERIOD_MS) != pdTRUE)
    {
        ESP_LOGE(TAG, "ESP-NOW message processing task internal error at line %d.", __LINE__);
    }
}

static void _processing(void *pvParameter)
{
    _queue_t queue = {0};
    while (xQueueReceive(_queue_handle, &queue, portMAX_DELAY) == pdTRUE)
    {
        esp_err_t err = ESP_OK;
        switch (queue.id)
        {
        case TO_SEND:
            ESP_LOGI(TAG, "Outgoing ESP-NOW data to MAC %02X:%02X:%02X:%02X:%02X:%02X processing begin.", MAC2STR(queue.data.mac_addr));
            esp_now_peer_info_t *peer = heap_caps_malloc(sizeof(esp_now_peer_info_t), MALLOC_CAP_8BIT);
            if (peer == NULL)
            {
                ESP_LOGE(TAG, "Outgoing ESP-NOW data processing fail. Memory allocation fail or no free memory in the heap.");
                heap_caps_free(queue.data.payload);
                break;
            }
            memset(peer, 0, sizeof(esp_now_peer_info_t));
            peer->ifidx = _init_config.wifi_interface;
            memcpy(peer->peer_addr, queue.data.mac_addr, 6);
            err = esp_now_add_peer(peer);
            if (err == ESP_ERR_ESPNOW_NO_MEM)
            {
                ESP_LOGE(TAG, "Outgoing ESP-NOW data processing fail. No free memory in the heap for adding peer.");
                heap_caps_free(queue.data.payload);
                heap_caps_free(peer);
                break;
            }
            else if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Outgoing ESP-NOW data processing fail. Internal error with adding peer.");
                heap_caps_free(queue.data.payload);
                heap_caps_free(peer);
                break;
            }
            zh_espnow_event_on_send_t *on_send = heap_caps_malloc(sizeof(zh_espnow_event_on_send_t), MALLOC_CAP_8BIT);
            if (on_send == NULL)
            {
                ESP_LOGE(TAG, "Outgoing ESP-NOW data processing fail. Memory allocation fail or no free memory in the heap.");
                heap_caps_free(queue.data.payload);
                heap_caps_free(peer);
                break;
            }
            memset(on_send, 0, sizeof(zh_espnow_event_on_send_t));
            memcpy(on_send->mac_addr, queue.data.mac_addr, 6);
        SEND:
            ++_attempts;
            err = esp_now_send(queue.data.mac_addr, queue.data.payload, queue.data.payload_len);
            if (err == ESP_ERR_ESPNOW_NO_MEM)
            {
                ESP_LOGE(TAG, "Sending ESP-NOW data fail. No free memory in the heap.");
                heap_caps_free(queue.data.payload);
                heap_caps_free(peer);
                heap_caps_free(on_send);
                ESP_LOGE(TAG, "Outgoing ESP-NOW data processing fail.");
                break;
            }
            else if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Sending ESP-NOW data fail. Internal error.");
                heap_caps_free(queue.data.payload);
                heap_caps_free(peer);
                heap_caps_free(on_send);
                ESP_LOGE(TAG, "Outgoing ESP-NOW data processing fail.");
                break;
            }
            else
            {
                ESP_LOGI(TAG, "Sending ESP-NOW data to MAC %02X:%02X:%02X:%02X:%02X:%02X success.", MAC2STR(queue.data.mac_addr));
            }
            EventBits_t bit = xEventGroupWaitBits(_event_group_handle, DATA_SEND_SUCCESS | DATA_SEND_FAIL, pdTRUE, pdFALSE, 50 / portTICK_PERIOD_MS);
            if ((bit & DATA_SEND_SUCCESS) != 0)
            {
                ESP_LOGI(TAG, "Confirmation message received. ESP-NOW message to MAC %02X:%02X:%02X:%02X:%02X:%02X sent success.", MAC2STR(queue.data.mac_addr));
                on_send->status = ZH_ESPNOW_SEND_SUCCESS;
                _attempts = 0;
            }
            else
            {
                if (_attempts < _init_config.attempts)
                {
                    goto SEND;
                }
                ESP_LOGE(TAG, "Confirmation message not received. ESP-NOW message to MAC %02X:%02X:%02X:%02X:%02X:%02X sent fail.", MAC2STR(queue.data.mac_addr));
                on_send->status = ZH_ESPNOW_SEND_FAIL;
                _attempts = 0;
            }
            ESP_LOGI(TAG, "Outgoing ESP-NOW data to MAC %02X:%02X:%02X:%02X:%02X:%02X processed success.", MAC2STR(queue.data.mac_addr));
            if (esp_event_post(ZH_ESPNOW, ZH_ESPNOW_ON_SEND_EVENT, on_send, sizeof(zh_espnow_event_on_send_t), portTICK_PERIOD_MS) != ESP_OK)
            {
                ESP_LOGE(TAG, "ESP-NOW message processing task internal error at line %d.", __LINE__);
            }
            heap_caps_free(queue.data.payload);
            esp_now_del_peer(peer->peer_addr);
            heap_caps_free(peer);
            heap_caps_free(on_send);
            break;
        case ON_RECV:
            ESP_LOGI(TAG, "Incoming ESP-NOW data from MAC %02X:%02X:%02X:%02X:%02X:%02X processing begin.", MAC2STR(queue.data.mac_addr));
            zh_espnow_event_on_recv_t *recv_data = (zh_espnow_event_on_recv_t *)&queue.data;
            ESP_LOGI(TAG, "Incoming ESP-NOW data from MAC %02X:%02X:%02X:%02X:%02X:%02X processed success.", MAC2STR(queue.data.mac_addr));
            if (esp_event_post(ZH_ESPNOW, ZH_ESPNOW_ON_RECV_EVENT, recv_data, recv_data->data_len + 7, portTICK_PERIOD_MS) != ESP_OK)
            {
                ESP_LOGE(TAG, "ESP-NOW message processing task internal error at line %d.", __LINE__);
            }
            break;
        default:
            break;
        }
    }
    vTaskDelete(NULL);
}