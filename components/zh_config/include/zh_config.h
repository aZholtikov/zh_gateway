#pragma once

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

#define ZH_HIGH true
#define ZH_LOW false

#define ZH_ON true
#define ZH_OFF false

#define ZH_ONLINE true
#define ZH_OFFLINE false

#define ZH_NOT_USED 0xFF
//***********************************************************************************//
#define ZH_DEVICE_TYPE                             \
    DF(ZHDT_NONE, "")                              \
    DF(ZHDT_GATEWAY, "gateway")                    \
    DF(ZHDT_SWITCH, "espnow_switch")               \
    DF(ZHDT_LED, "espnow_led")                     \
    DF(ZHDT_SENSOR, "espnow_sensor")               \
    DF(ZHDT_RF_GATEWAY, "espnow_rf_gateway")       \
    DF(ZHDT_CONTROL_PANEL, "espnow_control_panel") \
    DF(ZHDT_DIMMER, "espnow_dimmer")               \
    DF(ZHDT_TERRARIUM, "espnow_terrarium")         \
    DF(ZHDT_RF_SENSOR, "rf_sensor")                \
    DF(ZHDT_BINARY_SENSOR, "espnow_sensor")        \
    DF(ZHDT_MAX, "")

typedef enum zh_device_type_t
{
#define DF(_value, _name) _value,
    ZH_DEVICE_TYPE
#undef DF
} zh_device_type_t;

char *zh_get_device_type_value_name(zh_device_type_t value);
//***********************************************************************************//
#define ZH_PAYLOAD_TYPE                 \
    DF(ZHPT_NONE, "")                   \
    DF(ZHPT_ATTRIBUTES, "attributes")   \
    DF(ZHPT_KEEP_ALIVE, "status")       \
    DF(ZHPT_SET, "set")                 \
    DF(ZHPT_STATE, "state")             \
    DF(ZHPT_UPDATE, "update")           \
    DF(ZHPT_RESTART, "restart")         \
    DF(ZHPT_SYSTEM, "system")           \
    DF(ZHPT_CONFIG, "config")           \
    DF(ZHPT_FORWARD, "forward")         \
    DF(ZHPT_UPDATE_BEGIN, "")           \
    DF(ZHPT_UPDATE_PROGRESS, "")        \
    DF(ZHPT_UPDATE_END, "")             \
    DF(ZHPT_UPDATE_ERROR, "")           \
    DF(ZHPT_UPDATE_FAIL, "")            \
    DF(ZHPT_UPDATE_SUCCESS, "")         \
    DF(ZHPT_BRIGHTNESS, "brightness")   \
    DF(ZHPT_TEMPERATURE, "temperature") \
    DF(ZHPT_RGB, "rgb")                 \
    DF(ZHPT_EFFECT, "effect")           \
    DF(ZHPT_HARDWARE, "hardware")       \
    DF(ZHPT_MAX, "")

typedef enum zh_payload_type_t
{
#define DF(_value, _name) _value,
    ZH_PAYLOAD_TYPE
#undef DF
} zh_payload_type_t;

char *zh_get_payload_type_value_name(zh_payload_type_t value);
//***********************************************************************************//
#define HA_COMPONENT_TYPE                               \
    DF(HACT_NONE, "")                                   \
    DF(HACT_ALARM_CONTROL_PANEL, "alarm_control_panel") \
    DF(HACT_BINARY_SENSOR, "binary_sensor")             \
    DF(HACT_BUTTON, "button")                           \
    DF(HACT_CAMERA, "camera")                           \
    DF(HACT_COVER, "cover")                             \
    DF(HACT_DEVICE_TRACKER, "device_tracker")           \
    DF(HACT_FAN, "fan")                                 \
    DF(HACT_HUMIDIFIER, "humidifier")                   \
    DF(HACT_CLIMATE_HVAC, "climate")                    \
    DF(HACT_LIGHT, "light")                             \
    DF(HACT_LOCK, "lock")                               \
    DF(HACT_NUMBER, "number")                           \
    DF(HACT_SCENE, "scene")                             \
    DF(HACT_SELECT, "select")                           \
    DF(HACT_SENSOR, "sensor")                           \
    DF(HACT_SIREN, "siren")                             \
    DF(HACT_SWITCH, "switch")                           \
    DF(HACT_UPDATE, "update")                           \
    DF(HACT_TEXT, "text")                               \
    DF(HACT_VACUUM, "vacuum")                           \
    DF(HACT_MAX, "")

typedef enum ha_component_type_t
{
#define DF(_value, _name) _value,
    HA_COMPONENT_TYPE
#undef DF
} ha_component_type_t;

char *zh_get_component_type_value_name(ha_component_type_t value);
//***********************************************************************************//
#define HA_BINARY_SENSOR_DEVICE_CLASS               \
    DF(HABSDC_NONE, "")                             \
    DF(HABSDC_BATTERY, "battery")                   \
    DF(HABSDC_BATTERY_CHARGING, "battery_charging") \
    DF(HABSDC_CARBON_MONOXIDE, "carbon_monoxide")   \
    DF(HABSDC_COLD, "cold")                         \
    DF(HABSDC_CONNECTIVITY, "connectivity")         \
    DF(HABSDC_DOOR, "door")                         \
    DF(HABSDC_GARAGE_DOOR, "garage_door")           \
    DF(HABSDC_GAS, "gas")                           \
    DF(HABSDC_HEAT, "heat")                         \
    DF(HABSDC_LIGHT, "light")                       \
    DF(HABSDC_LOCK, "lock")                         \
    DF(HABSDC_MOISTURE, "moisture")                 \
    DF(HABSDC_MOTION, "motion")                     \
    DF(HABSDC_MOVING, "moving")                     \
    DF(HABSDC_OCCUPANCY, "occupancy")               \
    DF(HABSDC_OPENING, "opening")                   \
    DF(HABSDC_PLUG, "plug")                         \
    DF(HABSDC_POWER, "power")                       \
    DF(HABSDC_PRESENCE, "presence")                 \
    DF(HABSDC_PROBLEM, "problem")                   \
    DF(HABSDC_RUNNING, "running")                   \
    DF(HABSDC_SAFETY, "safety")                     \
    DF(HABSDC_SMOKE, "smoke")                       \
    DF(HABSDC_SOUND, "sound")                       \
    DF(HABSDC_TAMPER, "tamper")                     \
    DF(HABSDC_UPDATE, "update")                     \
    DF(HABSDC_VIBRATION, "vibration")               \
    DF(HABSDC_WINDOW, "window")                     \
    DF(HABSDC_MAX, "")

typedef enum ha_binary_sensor_device_class_t
{
#define DF(_value, _name) _value,
    HA_BINARY_SENSOR_DEVICE_CLASS
#undef DF
} ha_binary_sensor_device_class_t;

char *zh_get_binary_sensor_device_class_value_name(ha_binary_sensor_device_class_t value);
//***********************************************************************************//
#define HA_COVER_DEVICE_CLASS    \
    DF(HACDC_NONE, "")           \
    DF(HACDC_AWNING, "awning")   \
    DF(HACDC_BLIND, "blind")     \
    DF(HACDC_CURTAIN, "curtain") \
    DF(HACDC_DAMPER, "damper")   \
    DF(HACDC_DOOR, "door")       \
    DF(HACDC_GARAGE, "garage")   \
    DF(HACDC_GATE, "gate")       \
    DF(HACDC_SHADE, "shade")     \
    DF(HACDC_SHUTTER, "shutter") \
    DF(HACDC_WINDOW, "window")   \
    DF(HACDC_MAX, "")

typedef enum ha_cover_device_class_t
{
#define DF(_value, _name) _value,
    HA_COVER_DEVICE_CLASS
#undef DF
} ha_cover_device_class_t;

char *zh_get_cover_device_class_value_name(ha_cover_device_class_t value);
//***********************************************************************************//
#define HA_SENSOR_DEVICE_CLASS                                         \
    DF(HASDC_NONE, "")                                                 \
    DF(HASDC_APPARENT_POWER, "apparent_power")                         \
    DF(HASDC_AQI, "aqi")                                               \
    DF(HASDC_BATTERY, "battery")                                       \
    DF(HASDC_CARBON_DIOXIDE, "carbon_dioxide")                         \
    DF(HASDC_CARBON_MONOXIDE, "carbon_monoxide")                       \
    DF(HASDC_CURRENT, "current")                                       \
    DF(HASDC_DATE, "date")                                             \
    DF(HASDC_DISTANCE, "distance")                                     \
    DF(HASDC_DURATION, "duration")                                     \
    DF(HASDC_ENERGY, "energy")                                         \
    DF(HASDC_FREQUENCY, "frequency")                                   \
    DF(HASDC_GAS, "gas")                                               \
    DF(HASDC_HUMIDITY, "humidity")                                     \
    DF(HASDC_ILLUMINANCE, "illuminance")                               \
    DF(HASDC_MOISTURE, "moisture")                                     \
    DF(HASDC_MONETARY, "monetar")                                      \
    DF(HASDC_NITROGEN_DIOXIDE, "nitrogen_dioxide")                     \
    DF(HASDC_NITROGEN_MONOXIDE, "nitrogen_monoxide")                   \
    DF(HASDC_NITROUS_OXIDE, "nitrous_oxide")                           \
    DF(HASDC_OZONE, "ozone")                                           \
    DF(HASDC_PM1, "pm1")                                               \
    DF(HASDC_PM10, "pm10")                                             \
    DF(HASDC_PM25, "pm25")                                             \
    DF(HASDC_POWER_FACTOR, "power_factor")                             \
    DF(HASDC_POWER, "power")                                           \
    DF(HASDC_PRECIPITATION, "precipitation")                           \
    DF(HASDC_PRECIPITATION_INTENSITY, "precipitation_intensity")       \
    DF(HASDC_PRESSURE, "pressure")                                     \
    DF(HASDC_REACTIVE_POWER, "reactive_power")                         \
    DF(HASDC_SIGNAL_STRENGTH, "signal_strength")                       \
    DF(HASDC_SPEED, "speed")                                           \
    DF(HASDC_SULPHUR_DIOXIDE, "sulphur_dioxide")                       \
    DF(HASDC_TEMPERATURE, "temperature")                               \
    DF(HASDC_TIMESTAMP, "timestamp")                                   \
    DF(HASDC_VOLATILE_ORGANIC_COMPOUNDS, "volatile_organic_compounds") \
    DF(HASDC_VOLTAGE, "voltage")                                       \
    DF(HASDC_VOLUME, "volume")                                         \
    DF(HASDC_WATER, "water")                                           \
    DF(HASDC_WEIGHT, "weight")                                         \
    DF(HASDC_WIND_SPEED, "wind_speed")                                 \
    DF(HASDC_MAX, "")

typedef enum ha_sensor_device_class_t
{
#define DF(_value, _name) _value,
    HA_SENSOR_DEVICE_CLASS
#undef DF
} ha_sensor_device_class_t;

char *zh_get_sensor_device_class_value_name(ha_sensor_device_class_t value);
//***********************************************************************************//
#define HA_SWITCH_DEVICE_CLASS  \
    DF(HASWDC_NONE, "")         \
    DF(HASWDC_OUTLET, "outlet") \
    DF(HASWDC_SWITCH, "switch") \
    DF(HASWDC_MAX, "")

typedef enum ha_switch_device_class_t
{
#define DF(_value, _name) _value,
    HA_SWITCH_DEVICE_CLASS
#undef DF
} ha_switch_device_class_t;

char *zh_get_switch_device_class_value_name(ha_switch_device_class_t value);
//***********************************************************************************//
#define HA_ON_OFF_TYPE             \
    DF(HAONOFT_NONE, "")           \
    DF(HAONOFT_ON, "ON")           \
    DF(HAONOFT_OFF, "OFF")         \
    DF(HAONOFT_OPEN, "OPEN")       \
    DF(HAONOFT_CLOSE, "CLOSE")     \
    DF(HAONOFT_LOW, "LOW")         \
    DF(HAONOFT_MID, "MID")         \
    DF(HAONOFT_HIGH, "HIGH")       \
    DF(HAONOFT_ALARM, "ALARM")     \
    DF(HAONOFT_DRY, "DRY")         \
    DF(HAONOFT_MOTION, "MOTION")   \
    DF(HAONOFT_CONNECT, "CONNECT") \
    DF(HAONOFT_LEAKAGE, "LEAKAGE") \
    DF(HAONOFT_MAX, "")

typedef enum ha_on_off_type_t
{
#define DF(_value, _name) _value,
    HA_ON_OFF_TYPE
#undef DF
} ha_on_off_type_t;

char *zh_get_on_off_type_value_name(ha_on_off_type_t value);
//***********************************************************************************//
#define HA_CHIP_TYPE              \
    DF(HACHT_NONE, "")            \
    DF(HACHT_ESP32, "ESP32")      \
    DF(HACHT_ESP8266, "ESP8266")  \
    DF(HACHT_ESP32S2, "ESP32-S2") \
    DF(HACHT_ESP32S3, "ESP32-S3") \
    DF(HACHT_ESP32C2, "ESP32-C2") \
    DF(HACHT_ESP32C3, "ESP32-C3") \
    DF(HACHT_ESP32C6, "ESP32-C6") \
    DF(HACHT_MAX, "")

typedef enum ha_chip_type_t
{
#define DF(_value, _name) _value,
    HA_CHIP_TYPE
#undef DF
} ha_chip_type_t;

char *zh_get_chip_type_value_name(ha_chip_type_t value);
//***********************************************************************************//
#define HA_LED_EFFECT_TYPE \
    DF(HALET_NONE, "")     \
    DF(HALET_MAX, "")

typedef enum ha_led_effect_type_t
{
#define DF(_value, _name) _value,
    HA_LED_EFFECT_TYPE
#undef DF
} ha_led_effect_type_t;
//***********************************************************************************//
#define HA_LED_TYPE         \
    DF(HALT_NONE, "")       \
    DF(HALT_W, "W")         \
    DF(HALT_WW, "WW")       \
    DF(HALT_RGB, "RGB")     \
    DF(HALT_RGBW, "RGBW")   \
    DF(HALT_RGBWW, "RGBWW") \
    DF(HALT_MAX, "")

typedef enum ha_led_type_t
{
#define DF(_value, _name) _value,
    HA_LED_TYPE
#undef DF
} ha_led_type_t;

char *zh_get_led_type_value_name(ha_led_type_t value);
//***********************************************************************************//
#define HA_SENSOR_TYPE          \
    DF(HAST_NONE, "")           \
    DF(HAST_DS18B20, "DS18B20") \
    DF(HAST_DHT11, "DHT11")     \
    DF(HAST_DHT22, "DHT22")     \
    DF(HAST_GATEWAY, "GATEWAY") \
    DF(HAST_WINDOW, "WINDOW")   \
    DF(HAST_DOOR, "DOOR")       \
    DF(HAST_LEAKAGE, "LEAKAGE") \
    DF(HAST_MAX, "")

typedef enum ha_sensor_type_t
{
#define DF(_value, _name) _value,
    HA_SENSOR_TYPE
#undef DF
} ha_sensor_type_t;

char *zh_get_sensor_type_value_name(ha_sensor_type_t value);
//***********************************************************************************//
typedef struct zh_sensor_config_message_t
{
    uint8_t unique_id;
    ha_sensor_device_class_t sensor_device_class;
    char unit_of_measurement[5];
    uint8_t suggested_display_precision;
    uint16_t expire_after;
    bool enabled_by_default;
    bool force_update;
    uint8_t qos;
    bool retain;
} __attribute__((packed)) zh_sensor_config_message_t;

typedef struct zh_sensor_hardware_config_message_t
{
    ha_sensor_type_t sensor_type;
    uint8_t sensor_pin_1;
    uint8_t sensor_pin_2;
    uint8_t power_pin;
    uint16_t measurement_frequency;
    bool battery_power;
} __attribute__((packed)) zh_sensor_hardware_config_message_t;

typedef struct zh_binary_sensor_config_message_t
{
    uint8_t unique_id;
    ha_binary_sensor_device_class_t binary_sensor_device_class;
    ha_on_off_type_t payload_on;
    ha_on_off_type_t payload_off;
    uint16_t expire_after;
    uint16_t off_delay;
    bool enabled_by_default;
    bool force_update;
    uint8_t qos;
    bool retain;
} __attribute__((packed)) zh_binary_sensor_config_message_t;

typedef struct zh_led_config_message_t
{
    uint8_t unique_id;
    ha_led_type_t led_type;
    ha_on_off_type_t payload_on;
    ha_on_off_type_t payload_off;
    bool enabled_by_default;
    bool optimistic;
    uint8_t qos;
    bool retain;
} __attribute__((packed)) zh_led_config_message_t;

typedef struct zh_led_hardware_config_message_t
{
    ha_led_type_t led_type;
    uint8_t first_white_pin;
    uint8_t second_white_pin;
    uint8_t red_pin;
    uint8_t green_pin;
    uint8_t blue_pin;
} __attribute__((packed)) zh_led_hardware_config_message_t;

typedef struct zh_switch_config_message_t
{
    uint8_t unique_id;
    ha_switch_device_class_t device_class;
    ha_on_off_type_t payload_on;
    ha_on_off_type_t payload_off;
    bool enabled_by_default;
    bool optimistic;
    uint8_t qos;
    bool retain;
} __attribute__((packed)) zh_switch_config_message_t;

typedef struct zh_switch_hardware_config_message_t
{
    uint8_t relay_pin;
    bool relay_on_level;
    uint8_t led_pin;
    bool led_on_level;
    uint8_t int_button_pin;
    bool int_button_on_level;
    uint8_t ext_button_pin;
    bool ext_button_on_level;
    uint8_t sensor_pin;
    ha_sensor_type_t sensor_type;
} __attribute__((packed)) zh_switch_hardware_config_message_t;
//***********************************************************************************//
typedef struct zh_sensor_status_message_t
{
    ha_sensor_type_t sensor_type;
    float temperature;
    float humidity;
    float pressure;
    float quality;
    float voltage;
    float reserved_1; // Reserved for future development.
    float reserved_2; // Reserved for future development.
    float reserved_3; // Reserved for future development.
    float reserved_4; // Reserved for future development.
    float reserved_5; // Reserved for future development.
} __attribute__((packed)) zh_sensor_status_message_t;

typedef struct zh_binary_sensor_status_message_t
{
    ha_sensor_type_t sensor_type;
    ha_on_off_type_t connect;
    ha_on_off_type_t open;
    ha_on_off_type_t battery;
    ha_on_off_type_t leakage;
    ha_on_off_type_t reserved_1; // Reserved for future development.
    ha_on_off_type_t reserved_2; // Reserved for future development.
    ha_on_off_type_t reserved_3; // Reserved for future development.
    ha_on_off_type_t reserved_4; // Reserved for future development.
    ha_on_off_type_t reserved_5; // Reserved for future development.
} __attribute__((packed)) zh_binary_sensor_status_message_t;

typedef struct zh_led_status_message_t
{
    ha_on_off_type_t status;
    uint8_t brightness;
    uint16_t temperature;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    ha_led_effect_type_t effect; // Reserved for future development.
} __attribute__((packed)) zh_led_status_message_t;

typedef struct zh_switch_status_message_t
{
    ha_on_off_type_t status;
} __attribute__((packed)) zh_switch_status_message_t;
//***********************************************************************************//
typedef struct zh_attributes_message_t
{
    ha_chip_type_t chip_type;
    ha_sensor_type_t sensor_type;
    char flash_size[5];
    uint8_t cpu_frequency;
    uint32_t heap_size;
    uint32_t min_heap_size;
    uint8_t reset_reason;
    char app_name[32];
    char app_version[32];
    uint32_t uptime;
} __attribute__((packed)) zh_attributes_message_t;

typedef struct zh_keep_alive_message_t
{
    bool online_status;
    uint8_t message_frequency;
} __attribute__((packed)) zh_keep_alive_message_t;

typedef union zh_config_message_t
{
    zh_binary_sensor_config_message_t binary_sensor_config_message;
    zh_sensor_config_message_t sensor_config_message;
    zh_sensor_hardware_config_message_t sensor_hardware_config_message;
    zh_led_config_message_t led_config_message;
    zh_led_hardware_config_message_t led_hardware_config_message;
    zh_switch_config_message_t switch_config_message;
    zh_switch_hardware_config_message_t switch_hardware_config_message;
} __attribute__((packed)) zh_config_message_t;

typedef union zh_status_message_t
{
    zh_binary_sensor_status_message_t binary_sensor_status_message;
    zh_sensor_status_message_t sensor_status_message;
    zh_led_status_message_t led_status_message;
    zh_switch_status_message_t switch_status_message;
} __attribute__((packed)) zh_status_message_t;

typedef struct zh_espnow_ota_message_t
{
    ha_chip_type_t chip_type;
    char app_name[32];
    char app_version[32];
    uint16_t part;
    uint8_t data_len;
    uint8_t data[128];
} __attribute__((packed)) zh_espnow_ota_message_t;

typedef struct zh_espnow_ota_data_t
{
    ha_chip_type_t chip_type;
    zh_device_type_t device_type;
    char app_name[32];
    char app_version[32];
    uint8_t mac_addr[6];
} __attribute__((packed)) zh_espnow_ota_data_t;

typedef union zh_payload_data_t
{
    zh_attributes_message_t attributes_message;
    zh_keep_alive_message_t keep_alive_message;
    zh_config_message_t config_message;
    zh_status_message_t status_message;
    zh_espnow_ota_message_t espnow_ota_message;
} __attribute__((packed)) zh_payload_data_t;

typedef struct zh_espnow_data_t
{
    zh_device_type_t device_type;
    zh_payload_type_t payload_type;
    zh_payload_data_t payload_data;
} __attribute__((packed)) zh_espnow_data_t;