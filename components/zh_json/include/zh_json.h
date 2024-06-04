/**
 * @file
 * Header file for the zh_json component.
 *
 */

#pragma once

#include "stdio.h"
#include "string.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Main structure of json data.
     *
     */
    typedef struct
    {
        char **names;     ///< Array of pointers of json names. @note
        char **values;    ///< Array of pointers of json values. @note
        uint8_t capacity; ///< Maximum capacity of the json. @note Used to control the size of allocated memory for array of pointers of json items. Usually equal to the current number of items in the json. Automatically changes when items are added or deleted.
        uint8_t size;     ///< Number of items in the json. @note
    } zh_json_t;

    /**
     * @brief Initialize json.
     *
     * @param[in,out] json Pointer to structure of json.
     *
     * @return
     *              - ESP_OK if initialization was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     */
    esp_err_t zh_json_init(zh_json_t *json);

    /**
     * @brief Deinitialize json. Free all allocated memory.
     *
     * @param[in,out] json Pointer to structure of json.
     *
     * @return
     *              - ESP_OK if deinitialization was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     */
    esp_err_t zh_json_free(zh_json_t *json);

    /**
     * @brief Add item at end of json. If sufficient memory is not available then it will resize the memory.
     *
     * @param[in,out] json Pointer to structure of json.
     * @param[in] name Pointer to name for add.
     * @param[in] value Pointer to value for add.
     *
     * @return
     *              - ESP_OK if add was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     */
    esp_err_t zh_json_add(zh_json_t *json, char *name, char *value);

    /**
     * @brief Create json.
     *
     * @param[in,out] json Pointer to structure of json.
     * @param[out] buffer Pointer to buffer for json.
     *
     * @return
     *              - ESP_OK if creation was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     */
    esp_err_t zh_json_create(zh_json_t *json, char *buffer);

#ifdef __cplusplus
}
#endif
