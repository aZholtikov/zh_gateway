/**
 * @file
 * The main code of the zh_vector component.
 *
 */

#include "zh_vector.h"

static const char *TAG = "zh_vector";

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity);

esp_err_t zh_vector_init(zh_vector_t *vector, uint16_t unit, bool spiram)
{
    ESP_LOGI(TAG, "Vector initialization begin.");
    if (vector == NULL || unit == 0)
    {
        ESP_LOGE(TAG, "Vector initialization fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    if (vector->status == true)
    {
        if (vector->unit == unit)
        {
            goto ZH_VECTOR_INIT_EXIT;
        }
        else
        {
            ESP_LOGE(TAG, "Vector initialization fail. Vector already initialized with other item size.");
            return ESP_ERR_INVALID_STATE;
        }
    }
    vector->capacity = 0;
    vector->size = 0;
    vector->unit = unit;
    vector->status = true;
    vector->spi_ram = spiram;
    if (vector->spi_ram == true)
    {
#ifdef CONFIG_IDF_TARGET_ESP8266
        ESP_LOGW(TAG, "SPI RAM not supported. Will be used IRAM.");
        vector->spi_ram = false;
#else
#ifndef CONFIG_SPIRAM
        ESP_LOGW(TAG, "SPI RAM not initialized. Will be used IRAM.");
        vector->spi_ram = false;
#endif
#endif
    }
ZH_VECTOR_INIT_EXIT:
    if (vector->spi_ram == true)
    {
        ESP_LOGI(TAG, "Vector initialization success. Vector located in SPI RAM.");
    }
    else
    {
        ESP_LOGI(TAG, "Vector initialization success. Vector located in IRAM.");
    }
    return ESP_OK;
}

esp_err_t zh_vector_free(zh_vector_t *vector)
{
    ESP_LOGI(TAG, "Vector deletion begin.");
    if (vector == NULL)
    {
        ESP_LOGE(TAG, "Vector deletion fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    if (vector->status == false)
    {
        ESP_LOGE(TAG, "Vector deletion fail. Vector not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    for (uint16_t i = 0; i < vector->size; ++i)
    {
        heap_caps_free(vector->items[i]);
    }
    vector->status = false;
    ESP_LOGI(TAG, "Vector deletion success.");
    return ESP_OK;
}

esp_err_t zh_vector_get_size(zh_vector_t *vector)
{
    ESP_LOGI(TAG, "Getting vector size begin.");
    if (vector == NULL || vector->status == false)
    {
        ESP_LOGE(TAG, "Getting vector size fail. Invalid argument or vector not initialized.");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Getting vector size success. Size: %d", vector->size);
    return vector->size;
}

esp_err_t zh_vector_push_back(zh_vector_t *vector, void *item)
{
    ESP_LOGI(TAG, "Adding item to vector begin.");
    if (vector == NULL || item == NULL)
    {
        ESP_LOGE(TAG, "Adding item to vector fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    if (vector->status == false)
    {
        ESP_LOGE(TAG, "Adding item to vector fail. Vector not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (vector->capacity == vector->size)
    {
        if (_resize(vector, vector->capacity + 1) == ESP_ERR_NO_MEM)
        {
            ESP_LOGE(TAG, "Adding item to vector fail. Memory allocation fail or no free memory in the heap.");
            return ESP_ERR_NO_MEM;
        }
    }
    if (vector->spi_ram == true)
    {
        vector->items[vector->size] = heap_caps_malloc(vector->unit, MALLOC_CAP_SPIRAM);
    }
    else
    {
        if (vector->unit / sizeof(void *) == 0)
        {
            vector->items[vector->size] = heap_caps_malloc(vector->unit, MALLOC_CAP_32BIT);
        }
        else
        {
            vector->items[vector->size] = heap_caps_malloc(vector->unit, MALLOC_CAP_8BIT);
        }
    }
    if (vector->items[vector->size] == NULL)
    {
        ESP_LOGE(TAG, "Adding item to vector fail. Memory allocation fail or no free memory in the heap.");
        return ESP_ERR_NO_MEM;
    }
    memset(vector->items[vector->size], 0, vector->unit);
    memcpy(vector->items[vector->size++], item, vector->unit);
    ESP_LOGI(TAG, "Adding item to vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_change_item(zh_vector_t *vector, uint16_t index, void *item)
{
    ESP_LOGI(TAG, "Changing item in vector begin.");
    if (vector == NULL || item == NULL)
    {
        ESP_LOGE(TAG, "Changing item in vector fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    if (vector->status == false)
    {
        ESP_LOGE(TAG, "Changing item in vector fail. Vector not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (index < vector->size)
    {
        memcpy(vector->items[index], item, vector->unit);
        ESP_LOGI(TAG, "Changing item in vector success.");
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Changing item in vector fail. Index does not exist.");
    return ESP_FAIL;
}

void *zh_vector_get_item(zh_vector_t *vector, uint16_t index)
{
    ESP_LOGI(TAG, "Getting item from vector begin.");
    if (vector == NULL)
    {
        ESP_LOGE(TAG, "Getting item from vector fail. Invalid argument.");
        return NULL;
    }
    if (vector->status == false)
    {
        ESP_LOGE(TAG, "Getting item from vector fail. Vector not initialized.");
        return NULL;
    }
    if (index < vector->size)
    {
        void *item = vector->items[index];
        ESP_LOGI(TAG, "Getting item from vector success.");
        return item;
    }
    else
    {
        ESP_LOGE(TAG, "Getting item from vector fail. Index does not exist.");
        return NULL;
    }
}

esp_err_t zh_vector_delete_item(zh_vector_t *vector, uint16_t index)
{
    ESP_LOGI(TAG, "Deleting item in vector begin.");
    if (vector == NULL)
    {
        ESP_LOGE(TAG, "Deleting item in vector fail. Invalid argument.");
        return ESP_ERR_INVALID_ARG;
    }
    if (vector->status == false)
    {
        ESP_LOGE(TAG, "Deleting item in vector fail. Vector not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (index < vector->size)
    {
        heap_caps_free(vector->items[index]);
        for (uint8_t i = index; i < (vector->size - 1); ++i)
        {
            vector->items[i] = vector->items[i + 1];
            vector->items[i + 1] = NULL;
        }
        --vector->size;
        _resize(vector, vector->capacity - 1);
        ESP_LOGI(TAG, "Deleting item in vector success.");
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Deleting item in vector fail. Index does not exist.");
    return ESP_FAIL;
}

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity)
{
    ESP_LOGI(TAG, "Vector resize begin.");
    if (capacity == 0)
    {
        goto VECTOR_RESIZE_EXIT;
    }
    if (vector->spi_ram == true)
    {
        vector->items = heap_caps_realloc(vector->items, sizeof(void *) * capacity, MALLOC_CAP_SPIRAM);
    }
    else
    {
        vector->items = heap_caps_realloc(vector->items, sizeof(void *) * capacity, MALLOC_CAP_32BIT);
    }
    if (vector->items == NULL)
    {
        ESP_LOGE(TAG, "Vector resize fail. Memory allocation fail or no free memory in the heap.");
        return ESP_ERR_NO_MEM;
    }
VECTOR_RESIZE_EXIT:
    vector->capacity = capacity;
    ESP_LOGI(TAG, "Vector resize success. New capacity: %d", vector->capacity);
    return ESP_OK;
}