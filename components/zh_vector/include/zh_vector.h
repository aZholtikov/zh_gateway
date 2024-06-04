/**
 * @file
 * Header file for the zh_vector component.
 *
 */

#pragma once

#include "stdlib.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Main structure of vector data.
     *
     */
    typedef struct
    {
        void **items;      ///< Array of pointers of vector items. @note
        uint16_t capacity; ///< Maximum capacity of the vector. @note Used to control the size of allocated memory for array of pointers of vector items. Usually equal to the current number of items in the vector. Automatically changes when items are added or deleted.
        uint16_t size;     ///< Number of items in the vector. @note Can be read with zh_vector_get_size().
        uint16_t unit;     ///< Vector item size. @note Possible values from 1 to 65536.
        bool status;       ///< Vector initialization status flag. @note Used to prevent execution of vector functions without prior vector initialization.
        bool spi_ram;      ///< SPI RAM using status flag. @note True - vector will be placed in SPI RAM, false - vector will be placed in RAM.
    } zh_vector_t;

    /**
     * @brief Initialize vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] unit Size of vector item.
     * @param[in] spiram SPI RAM using (true - vector will be placed in SPI RAM, false - vector will be placed in RAM).
     *
     * @attention For using SPI RAM select “Make RAM allocatable using heap_caps_malloc(…, MALLOC_CAP_SPIRAM)” from CONFIG_SPIRAM_USE. For ESP32 with external, SPI-connected RAM only.
     *
     * @note If SPI RAM is not supported or not initialised via menuconfig vector will be placed in RAM regardless of the set spiram value.
     *
     * @return
     *              - ESP_OK if initialization was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_INVALID_STATE if vector already initialized with other item size
     */
    esp_err_t zh_vector_init(zh_vector_t *vector, uint16_t unit, bool spiram);

    /**
     * @brief Deinitialize vector. Free all allocated memory.
     *
     * @param[in] vector Pointer to main structure of vector data.
     *
     * @return
     *              - ESP_OK if deinitialization was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_INVALID_STATE if vector not initialized
     */
    esp_err_t zh_vector_free(zh_vector_t *vector);

    /**
     * @brief Get current vector size.
     *
     * @param[in] vector Pointer to main structure of vector data.
     *
     * @return
     *              - Vector size
     *              - ESP_FAIL if parameter error or vector not initialized
     */
    esp_err_t zh_vector_get_size(zh_vector_t *vector);

    /**
     * @brief Add item at end of vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] item Pointer to item for add.
     *
     * @return
     *              - ESP_OK if add was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_NO_MEM if memory allocation fail or no free memory in the heap
     *              - ESP_ERR_INVALID_STATE if vector not initialized
     */
    esp_err_t zh_vector_push_back(zh_vector_t *vector, void *item);

    /**
     * @brief Change item by index.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for change.
     * @param[in] item Pointer to new data of item.
     *
     * @return
     *              - ESP_OK if change was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_INVALID_STATE if vector not initialized
     *              - ESP_FAIL if index does not exist
     */
    esp_err_t zh_vector_change_item(zh_vector_t *vector, uint16_t index, void *item);

    /**
     * @brief Get item by index.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for get.
     *
     * @return
     *              - Pointer to item
     *              - NULL if parameter error or vector not initialized or if index does not exist
     */
    void *zh_vector_get_item(zh_vector_t *vector, uint16_t index);

    /**
     * @brief Delete item by index and shifts all elements in vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for delete.
     *
     * @return
     *              - ESP_OK if delete was success
     *              - ESP_ERR_INVALID_ARG if parameter error
     *              - ESP_ERR_INVALID_STATE if vector not initialized
     *              - ESP_FAIL if index does not exist
     */
    esp_err_t zh_vector_delete_item(zh_vector_t *vector, uint16_t index);

#ifdef __cplusplus
}
#endif