/**
 * @file
 * The main code of the zh_json component.
 *
 */

#include "zh_json.h"

static void _resize(zh_json_t *json, uint8_t capacity);
static void *_get_name(zh_json_t *json, uint8_t index);
static void *_get_value(zh_json_t *json, uint8_t index);

esp_err_t zh_json_init(zh_json_t *json)
{
    if (json == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    json->capacity = 0;
    json->size = 0;
    json->names = calloc(json->capacity, sizeof(char *));
    json->values = calloc(json->capacity, sizeof(char *));
    return ESP_OK;
}

esp_err_t zh_json_free(zh_json_t *json)
{
    if (json == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    free(json->names);
    free(json->values);
    return ESP_OK;
}

static void _resize(zh_json_t *json, uint8_t capacity)
{
    char **name = realloc(json->names, sizeof(char *) * capacity);
    char **value = realloc(json->values, sizeof(char *) * capacity);
    json->names = name;
    json->values = value;
    json->capacity = capacity;
}

static void *_get_name(zh_json_t *json, uint8_t index)
{
    char *name = NULL;
    if (index < json->size)
    {
        name = json->names[index];
    }
    return name;
}

static void *_get_value(zh_json_t *json, uint8_t index)
{
    char *value = NULL;
    if (index < json->size)
    {
        value = json->values[index];
    }
    return value;
}

esp_err_t zh_json_add(zh_json_t *json, char *name, char *value)
{
    if (json == NULL || name == NULL || value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (json->capacity == json->size)
    {
        _resize(json, json->capacity + 1);
    }
    json->names[json->size] = name;
    json->values[json->size++] = value;
    return ESP_OK;
}

esp_err_t zh_json_create(zh_json_t *json, char *buffer)
{
    if (json == NULL || buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    strcat(buffer, "{ ");
    for (uint8_t i = 0; i < json->size; ++i)
    {
        strcat(buffer, "\"");
        strcat(buffer, _get_name(json, i));
        strcat(buffer, "\": \"");
        strcat(buffer, _get_value(json, i));
        strcat(buffer, "\"");
        if (i != json->size - 1)
        {
            strcat(buffer, ", ");
        }
    }
    strcat(buffer, " }");
    return ESP_OK;
}