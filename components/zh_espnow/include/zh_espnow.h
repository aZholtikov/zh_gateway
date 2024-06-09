/**
 * @file
 * Header file for the zh_espnow component.
 *
 */

#pragma once

#include "string.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

/**
 * @brief Unique identifier of ESP-NOW interface events base. Used when registering the event handler.
 *
 */
#define ESP_EVENT_BASE ZH_ESPNOW

/**
 * @brief Default values for zh_espnow_init_config_t structure for initial initialization of ESP-NOW interface.
 *
 */
#define ZH_ESPNOW_INIT_CONFIG_DEFAULT() \
    {                                   \
        .task_priority = 4,             \
        .stack_size = 2048,             \
        .queue_size = 32,               \
        .wifi_interface = WIFI_IF_STA,  \
        .wifi_channel = 1,              \
        .attempts = 3                   \
    }

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Structure for initial initialization of ESP-NOW interface.
     *
     * @note Before initialize ESP-NOW interface recommend initialize zh_espnow_init_config_t structure with default values.
     *
     * @code zh_espnow_init_config_t config = ZH_ESPNOW_INIT_CONFIG_DEFAULT() @endcode
     */
    typedef struct
    {
        uint8_t task_priority;           ///< Task priority for the ESP-NOW messages processing. @note It is not recommended to set a value less than 4.
        uint16_t stack_size;             ///< Stack size for task for the ESP-NOW messages processing. @note The minimum size is 2048 bytes.
        uint8_t queue_size;              ///< Queue size for task for the ESP-NOW messages processing. @note The size depends on the number of messages to be processed. It is not recommended to set the value less than 16.
        wifi_interface_t wifi_interface; ///< WiFi interface (STA or AP) used for ESP-NOW operation. @note The MAC address of the device depends on the selected WiFi interface.
        uint8_t wifi_channel;            ///< Wi-Fi channel uses to send/receive ESP-NOW data. @note Values from 1 to 14.
        uint8_t attempts;                ///< Maximum number of attempts to send a message. @note It is not recommended to set a value greater than 5.
    } zh_espnow_init_config_t;

    /// \cond
    ESP_EVENT_DECLARE_BASE(ESP_EVENT_BASE);
    /// \endcond

    /**
     * @brief Enumeration of possible ESP-NOW events.
     *
     */
    typedef enum
    {
        ZH_ESPNOW_ON_RECV_EVENT, ///< The event when the ESP-NOW message was received.
        ZH_ESPNOW_ON_SEND_EVENT  ///< The event when the ESP-NOW message was sent.
    } zh_espnow_event_type_t;

    /**
     * @brief Enumeration of possible status of sent ESP-NOW message.
     *
     */
    typedef enum
    {
        ZH_ESPNOW_SEND_SUCCESS, ///< If ESP-NOW message was sent success.
        ZH_ESPNOW_SEND_FAIL     ///< If ESP-NOW message was sent fail.
    } zh_espnow_on_send_event_type_t;

    /**
     * @brief Structure for sending data to the event handler when an ESP-NOW message was sent.
     *
     * @note Should be used with ZH_ESPNOW event base and ZH_ESPNOW_ON_SEND_EVENT event.
     */
    typedef struct
    {
        uint8_t mac_addr[6];                   ///< MAC address of the device to which the ESP-NOW message was sent. @note
        zh_espnow_on_send_event_type_t status; ///< Status of sent ESP-NOW message. @note
    } zh_espnow_event_on_send_t;

    /**
     * @brief Structure for sending data to the event handler when an ESP-NOW message was received.
     *
     * @note Should be used with ZH_ESPNOW event base and ZH_ESPNOW_ON_RECV_EVENT event.
     */
    typedef struct
    {
        uint8_t mac_addr[6]; ///< MAC address of the sender ESP-NOW message. @note
        uint8_t *data;       ///< Pointer to the data of the received ESP-NOW message. @note
        uint8_t data_len;    ///< Size of the received ESP-NOW message. @note
    } zh_espnow_event_on_recv_t;

    /**
     * @brief Initialize ESP-NOW interface.
     *
     * @note Before initialize ESP-NOW interface recommend initialize zh_espnow_init_config_t structure with default values.
     *
     * @code zh_espnow_init_config_t config = ZH_ESPNOW_INIT_CONFIG_DEFAULT() @endcode
     *
     * @param[in] config Pointer to ESP-NOW initialized configuration structure. Can point to a temporary variable.
     *
     * @return
     *              - ESP_OK if initialization was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_WIFI_NOT_INIT if WiFi is not initialized
     *              - ESP_FAIL if any internal error
     */
    esp_err_t zh_espnow_init(const zh_espnow_init_config_t *config);

    /**
     * @brief Deinitialize ESP-NOW interface.
     *
     * @return
     *              - ESP_OK if deinitialization was success
     *              - ESP_FAIL if ESP-NOW is not initialized
     */
    esp_err_t zh_espnow_deinit(void);

    /**
     * @brief Send ESP-NOW data.
     *
     * @param[in] target Pointer to a buffer containing an eight-byte target MAC. Can be NULL for broadcast.
     * @param[in] data Pointer to a buffer containing the data for send.
     * @param[in] data_len Sending data length.
     *
     * @note The function will return an ESP_ERR_INVALID_STATE error if less than 10% of the size set at initialization remains in the message queue.
     *
     * @return
     *              - ESP_OK if sent was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_NO_MEM if memory allocation fail or no free memory in the heap
     *              - ESP_ERR_INVALID_STATE if queue for outgoing data is almost full
     *              - ESP_FAIL if ESP-NOW is not initialized or any internal error
     */
    esp_err_t zh_espnow_send(const uint8_t *target, const uint8_t *data, const uint8_t data_len);

#ifdef __cplusplus
}
#endif