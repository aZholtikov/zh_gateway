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

typedef enum // Enumeration of device types supported by the ESP-NOW gateway.
{
#define DF(_value, _name) _value,
    ZH_DEVICE_TYPE
#undef DF
} zh_device_type_t;

/**
 * @brief Get char description from the enumeration zh_device_type_t value.
 *
 * @note Used for identificate device type in MQTT topics (example - "homeassistant/espnow_switch/70-03-9F-44-BE-F7").
 *
 * @param[in] value Enumeration value of zh_device_type_t.
 *
 * @return Pointer to char value
 */
char *zh_get_device_type_value_name(zh_device_type_t value);

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

typedef enum // Enumeration of payload types supported by the ESP-NOW gateway.
{
#define DF(_value, _name) _value,
    ZH_PAYLOAD_TYPE
#undef DF
} zh_payload_type_t;

/**
 * @brief Get char description from the enumeration zh_payload_type_t value.
 *
 * @note Used for identificate payload type in MQTT topics (example - "homeassistant/espnow_switch/70-03-9F-44-BE-F7/attributes").
 *
 * @param[in] value Enumeration value of zh_payload_type_t.
 *
 * @return Pointer to char value
 */
char *zh_get_payload_type_value_name(zh_payload_type_t value);

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

typedef enum // Enumeration of device types supported by the Home Assistant. For details see https://www.home-assistant.io/integrations/mqtt.
{
#define DF(_value, _name) _value,
    HA_COMPONENT_TYPE
#undef DF
} ha_component_type_t;

/**
 * @brief Get char description from the enumeration ha_component_type_t value.
 *
 * @note Used to prepare a configuration message for Home Assistant MQTT discovery.
 *
 * @param[in] value Enumeration value of ha_component_type_t.
 *
 * @return Pointer to char value
 */
char *zh_get_component_type_value_name(ha_component_type_t value);

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

typedef enum // Enumeration of binary sensor types supported by the Home Assistant. For details see https://www.home-assistant.io/integrations/binary_sensor/#device-class.
{
#define DF(_value, _name) _value,
    HA_BINARY_SENSOR_DEVICE_CLASS
#undef DF
} ha_binary_sensor_device_class_t;

/**
 * @brief Get char description from the enumeration ha_binary_sensor_device_class_t value.
 *
 * @note Used to prepare a configuration message for Home Assistant MQTT discovery.
 *
 * @param[in] value Enumeration value of ha_binary_sensor_device_class_t.
 *
 * @return Pointer to char value
 */
char *zh_get_binary_sensor_device_class_value_name(ha_binary_sensor_device_class_t value);

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

typedef enum // Enumeration of cover types supported by the Home Assistant. For details see https://www.home-assistant.io/integrations/cover.
{
#define DF(_value, _name) _value,
    HA_COVER_DEVICE_CLASS
#undef DF
} ha_cover_device_class_t;

/**
 * @brief Get char description from the enumeration ha_cover_device_class_t value.
 *
 * @note Used to prepare a configuration message for Home Assistant MQTT discovery.
 *
 * @param[in] value Enumeration value of ha_cover_device_class_t.
 *
 * @return Pointer to char value
 */
char *zh_get_cover_device_class_value_name(ha_cover_device_class_t value);

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

typedef enum // Enumeration of sensor types supported by the Home Assistant. For details see https://www.home-assistant.io/integrations/sensor.
{
#define DF(_value, _name) _value,
    HA_SENSOR_DEVICE_CLASS
#undef DF
} ha_sensor_device_class_t;

/**
 * @brief Get char description from the enumeration ha_sensor_device_class_t value.
 *
 * @note Used to prepare a configuration message for Home Assistant MQTT discovery.
 *
 * @param[in] value Enumeration value of ha_sensor_device_class_t.
 *
 * @return Pointer to char value
 */
char *zh_get_sensor_device_class_value_name(ha_sensor_device_class_t value);

#define HA_SWITCH_DEVICE_CLASS  \
    DF(HASWDC_NONE, "")         \
    DF(HASWDC_OUTLET, "outlet") \
    DF(HASWDC_SWITCH, "switch") \
    DF(HASWDC_MAX, "")

typedef enum // Enumeration of switch types supported by the Home Assistant. For details see https://www.home-assistant.io/integrations/switch.
{
#define DF(_value, _name) _value,
    HA_SWITCH_DEVICE_CLASS
#undef DF
} ha_switch_device_class_t;

/**
 * @brief Get char description from the enumeration ha_switch_device_class_t value.
 *
 * @note Used to prepare a configuration message for Home Assistant MQTT discovery.
 *
 * @param[in] value Enumeration value of ha_switch_device_class_t.
 *
 * @return Pointer to char value
 */
char *zh_get_switch_device_class_value_name(ha_switch_device_class_t value);

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

typedef enum // Enumeration of payload_on / payload_off types supported by gateway.
{
#define DF(_value, _name) _value,
    HA_ON_OFF_TYPE
#undef DF
} ha_on_off_type_t;

/**
 * @brief Get char description from the enumeration ha_on_off_type_t value.
 *
 * @note Used to prepare a configuration message for Home Assistant MQTT discovery.
 *
 * @param[in] value Enumeration value of ha_on_off_type_t.
 *
 * @return Pointer to char value
 */
char *zh_get_on_off_type_value_name(ha_on_off_type_t value);

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

typedef enum // Enumeration of ESP module types supported by gateway.
{
#define DF(_value, _name) _value,
    HA_CHIP_TYPE
#undef DF
} ha_chip_type_t;

/**
 * @brief Get char description from the enumeration ha_chip_type_t value.
 *
 * @note Used to prepare a attribytes message by ESP-NOW gateway.
 *
 * @param[in] value Enumeration value of ha_chip_type_t.
 *
 * @return Pointer to char value
 */
char *zh_get_chip_type_value_name(ha_chip_type_t value);

#define HA_LED_EFFECT_TYPE \
    DF(HALET_NONE, "")     \
    DF(HALET_MAX, "")

typedef enum // Enumeration of light effect supported by gateway.
{
#define DF(_value, _name) _value,
    HA_LED_EFFECT_TYPE
#undef DF
} ha_led_effect_type_t;

#define HA_LED_TYPE                                                       \
    DF(HALT_NONE, "")                                                     \
    DF(HALT_W, "Cold white or Warm white or one another color")           \
    DF(HALT_WW, "Cold white + Warm white")                                \
    DF(HALT_RGB, "Red + Green + Blue colors")                             \
    DF(HALT_RGBW, "Red + Green + Blue + Cold white or Warm white colors") \
    DF(HALT_RGBWW, "Red + Green + Blue + Cold white + Warm white colors") \
    DF(HALT_MAX, "")

typedef enum // Enumeration of led types supported by gateway.
{
#define DF(_value, _name) _value,
    HA_LED_TYPE
#undef DF
} ha_led_type_t;

#define HA_SENSOR_TYPE          \
    DF(HAST_NONE, "")           \
    DF(HAST_DS18B20, "DS18B20") \
    DF(HAST_DHT11, "DHT11")     \
    DF(HAST_DHT22, "DHT22")     \
    DF(HAST_GATEWAY, "")        \
    DF(HAST_WINDOW, "")         \
    DF(HAST_DOOR, "")           \
    DF(HAST_LEAKAGE, "")        \
    DF(HAST_MAX, "")

typedef enum // Enumeration of sensor / binary sensor supported by gateway.
{
#define DF(_value, _name) _value,
    HA_SENSOR_TYPE
#undef DF
} ha_sensor_type_t;

/**
 * @brief Get char description from the enumeration ha_sensor_type_t value.
 *
 * @note Used to prepare at attributes messages and status messages by ESP-NOW gateway.
 *
 * @param[in] value Enumeration value of ha_sensor_type_t.
 *
 * @return Pointer to char value
 */
char *zh_get_sensor_type_value_name(ha_sensor_type_t value);

typedef struct // Structure for data exchange between ESP-NOW devices.
{
    zh_device_type_t device_type;   // Type of ESP-NOW message sender (gateway, switch, led, etc…).
    zh_payload_type_t payload_type; // Type of payload for indicating to the recipient of the message into which required structure the received data should be converted (if required).
    union                           // Main union for data exchange between ESP-NOW devices. @attention Usually not used in this view. According to the device_type data and payload_type data, the ESP-NOW device should convert the payload_data to the required secondary structure/union (excluding the case of having to send an empty message).
    {
        struct // Secondary structure of attributes message.
        {
            ha_chip_type_t chip_type;     // Used ESP module type.
            ha_sensor_type_t sensor_type; // Used sensor/binary sensor type (if present).
            char flash_size[5];           // SoC flash memory.
            uint8_t cpu_frequency;        // SoC frequency.
            uint32_t heap_size;           // Current HEAP memory size.
            uint32_t min_heap_size;       // Minimum HEAP memory size.
            uint8_t reset_reason;         // Last reset reason.
            char app_name[32];            // Firmware application name.
            char app_version[32];         // Firmware application version.
            uint32_t uptime;              // Uptime work (in seconds).
        } attributes_message;
        struct // Secondary structure of keep alive message.
        {
            bool online_status;        // Current status of ESP-NOW device operation. @note Online (true) / Offline (false).
            uint8_t message_frequency; // Frequency of transmission of the keep alive message by ESP-NOW device. @note Used by the ESP-NOW gateway to set the offline status of a ESP-NOW node when the message sending time is exceeded.
        } keep_alive_message;
        union // Secondary union of structures of any configuration messages. @attention Not used in this view. Should be converted to the required tertiary structure.
        {
            struct // Tertiary structure of zh_espnow_binary_sensor node configuration message. @note Used for publish at MQTT zh_espnow_binary_sensor node configuration message.
            {
                uint8_t unique_id;                                          // An ID that uniquely identifies this binary sensor device. @note The ID will look like this - "MAC-X" (for example 64-B7-08-31-00-A8-1). @attention If two binary sensors have the same unique ID, Home Assistant will raise an exception.
                ha_binary_sensor_device_class_t binary_sensor_device_class; // Binary sensor type supported by the Home Assistant. @note Used to prepare a correct configuration message for Home Assistant MQTT discovery. For details see https://www.home-assistant.io/integrations/binary_sensor.
                ha_on_off_type_t payload_on;                                // The payload that represents ON state.
                ha_on_off_type_t payload_off;                               // The payload that represents OFF state.
                uint16_t expire_after;                                      // If set, it defines the number of seconds after the sensors state expires, if its not updated. After expiry, the sensors state becomes unavailable.
                uint16_t off_delay;                                         // For sensors that only send on state updates (like PIRs), this variable sets a delay in seconds after which the sensors state will be updated back to off.
                bool enabled_by_default;                                    // Flag which defines if the entity should be enabled when first added.
                bool force_update;                                          // Sends update events (which results in update of state objects last_changed) even if the sensors state hasnt changed. Useful if you want to have meaningful value graphs in history or want to create an automation that triggers on every incoming state message (not only when the sensors new state is different to the current one).
                uint8_t qos;                                                // The maximum QoS level to be used when receiving and publishing messages.
                bool retain;                                                // If the published message should have the retain flag on or not.
            } binary_sensor_config_message;
            struct // Tertiary structure of zh_espnow_sensor node configuration message. @note Used publish at MQTT zh_espnow_sensor node configuration message.
            {
                uint8_t unique_id;                            // An ID that uniquely identifies this sensor device. @note The ID will look like this - "MAC-X" (for example 64-B7-08-31-00-A8-1). @attention If two sensors have the same unique ID, Home Assistant will raise an exception.
                ha_sensor_device_class_t sensor_device_class; // Sensor type supported by the Home Assistant. @note Used to prepare a correct configuration message for Home Assistant MQTT discovery. For details see https://www.home-assistant.io/integrations/sensor.
                char unit_of_measurement[5];                  // Defines the units of measurement of the sensor, if any.
                uint8_t suggested_display_precision;          // The number of decimals which should be used in the sensors state after rounding.
                uint16_t expire_after;                        // If set, it defines the number of seconds after the sensors state expires, if its not updated. After expiry, the sensors state becomes unavailable.
                bool enabled_by_default;                      // Flag which defines if the entity should be enabled when first added.
                bool force_update;                            // Sends update events (which results in update of state objects last_changed) even if the sensors state hasnt changed. Useful if you want to have meaningful value graphs in history or want to create an automation that triggers on every incoming state message (not only when the sensors new state is different to the current one).
                uint8_t qos;                                  // The maximum QoS level to be used when receiving and publishing messages.
                bool retain;                                  // If the published message should have the retain flag on or not.
            } sensor_config_message;
            struct // Tertiary structure of zh_espnow_sensor node hardware configuration message. @note Used for change hardware configuration / publish at MQTT zh_espnow_sensor node hardware configuration message.
            {
                ha_sensor_type_t sensor_type;   // Sensor types. @note Used in zh_espnow_sensor firmware only.
                uint8_t sensor_pin_1;           // Sensor GPIO number 1. @note Main pin for 1-wire sensors, SDA pin for I2C sensors.
                uint8_t sensor_pin_2;           // Sensor GPIO number 2. @note SCL pin for I2C sensors.
                uint8_t power_pin;              // Power GPIO number (if used sensor power control).
                uint16_t measurement_frequency; // Measurement frequency (sleep time on battery powering).
                bool battery_power;             // Battery powered. @note Battery powering (true) / external powering (false).
            } sensor_hardware_config_message;
            struct // Tertiary structure of zh_espnow_led node configuration message. @note Used for publish at MQTT zh_espnow_led node configuration message.
            {
                uint8_t unique_id;            // An ID that uniquely identifies this light device. @note The ID will look like this - "MAC-X" (for example 64-B7-08-31-00-A8-1). @attention If two lights have the same unique ID, Home Assistant will raise an exception.
                ha_led_type_t led_type;       // Led type. @note Used to identify the led type by ESP-NOW gateway and prepare a correct configuration message for Home Assistant MQTT discovery.
                ha_on_off_type_t payload_on;  // The payload that represents ON state.
                ha_on_off_type_t payload_off; // The payload that represents OFF state.
                bool enabled_by_default;      // Flag which defines if the entity should be enabled when first added.
                bool optimistic;              // Flag that defines if led works in optimistic mode.
                uint8_t qos;                  // The maximum QoS level to be used when receiving and publishing messages.
                bool retain;                  // If the published message should have the retain flag on or not.
            } led_config_message;
            struct // Tertiary structure of zh_espnow_led node hardware configuration message. @note Used for change hardware configuration / publish at MQTT zh_espnow_led node hardware configuration message.
            {
                ha_led_type_t led_type;   // Led types. @note Used in zh_espnow_led firmware only.
                uint8_t first_white_pin;  // First white GPIO number.
                uint8_t second_white_pin; // Second white GPIO number (if present).
                uint8_t red_pin;          // Red GPIO number (if present).
                uint8_t green_pin;        // Green GPIO number (if present).
                uint8_t blue_pin;         // Blue GPIO number (if present).
            } led_hardware_config_message;
            struct // Tertiary structure of zh_espnow_switch node configuration message. @note Used for publish at MQTT zh_espnow_switch node configuration message.
            {
                uint8_t unique_id;                     // An ID that uniquely identifies this switch device. @note The ID will look like this - "MAC-X" (for example 64-B7-08-31-00-A8-1). @attention If two switches have the same unique ID, Home Assistant will raise an exception.
                ha_switch_device_class_t device_class; // Switch type supported by the Home Assistant. @note Used to prepare a correct configuration message for Home Assistant MQTT discovery. For details see https://www.home-assistant.io/integrations/switch
                ha_on_off_type_t payload_on;           // The payload that represents ON state.
                ha_on_off_type_t payload_off;          // The payload that represents OFF state.
                bool enabled_by_default;               // Flag which defines if the entity should be enabled when first added.
                bool optimistic;                       // Flag that defines if switch works in optimistic mode.
                uint8_t qos;                           // The maximum QoS level to be used when receiving and publishing messages.
                bool retain;                           // If the published message should have the retain flag on or not.
            } switch_config_message;
            struct // Tertiary structure of zh_espnow_switch node hardware configuration message. @note Used for change hardware configuration / publish at MQTT zh_espnow_switch node hardware configuration message.
            {
                uint8_t relay_pin;            // Relay GPIO number.
                bool relay_on_level;          // Relay ON level. @note HIGH (true) / LOW (false).
                uint8_t led_pin;              // Led GPIO number (if present).
                bool led_on_level;            // Led ON level (if present). @note HIGH (true) / LOW (false).
                uint8_t int_button_pin;       // Internal button GPIO number (if present).
                bool int_button_on_level;     // Internal button trigger level (if present). @note HIGH (true) / LOW (false).
                uint8_t ext_button_pin;       // External button GPIO number (if present).
                bool ext_button_on_level;     // External button trigger level (if present). @note HIGH (true) / LOW (false).
                uint8_t sensor_pin;           // Sensor GPIO number (if present).
                ha_sensor_type_t sensor_type; // Sensor types (if present). @note Used to identify the sensor type by ESP-NOW gateway and send the appropriate sensor status messages to MQTT.
            } switch_hardware_config_message;
        } config_message;
        union // Secondary union of structures of any status messages. @attention Not used in this view. Should be converted to the required tertiary structure.
        {
            struct // Tertiary structure of zh_espnow_binary_sensor node status message.
            {
                ha_sensor_type_t sensor_type; // Binary sensor types.  @note Used to identify the binary sensor type by ESP-NOW gateway and send the appropriate binary sensor status messages to MQTT.
                ha_on_off_type_t connect;     // Event that caused the sensor to be triggered (if present). @note Example - CONNECT @attention Must be same with set on binary_sensor_config_message structure.
                ha_on_off_type_t open;        // Event that caused the sensor to be triggered (if present). @note Example - OPEN / CLOSE @attention Must be same with set on binary_sensor_config_message structure.
                ha_on_off_type_t battery;     // Event that caused the sensor to be triggered (if present). @note Example - HIGH / LOW @attention Must be same with set on binary_sensor_config_message structure.
                ha_on_off_type_t leakage;     // Event that caused the sensor to be triggered (if present). @note Example - DRY / LEAKAGE @attention Must be same with set on binary_sensor_config_message structure.
                ha_on_off_type_t reserved_1;  // Reserved for future development.
                ha_on_off_type_t reserved_2;  // Reserved for future development.
                ha_on_off_type_t reserved_3;  // Reserved for future development.
                ha_on_off_type_t reserved_4;  // Reserved for future development.
                ha_on_off_type_t reserved_5;  // Reserved for future development.
            } binary_sensor_status_message;
            struct // Tertiary structure of zh_espnow_sensor node status message.
            {
                ha_sensor_type_t sensor_type; // Sensor types. @note Used to identify the sensor type by ESP-NOW gateway and send the appropriate sensor status messages to MQTT.
                float temperature;            // Temperature value (if present).
                float humidity;               // Humidity value (if present).
                float pressure;               // Pressure value (if present).
                float quality;                // Quality value (if present).
                float voltage;                // Voltage value (if present).
                float reserved_1;             // Reserved for future development.
                float reserved_2;             // Reserved for future development.
                float reserved_3;             // Reserved for future development.
                float reserved_4;             // Reserved for future development.
                float reserved_5;             // Reserved for future development.
            } sensor_status_message;
            struct // Tertiary structure of zh_espnow_led node status message.
            {
                ha_on_off_type_t status;     // Status of the zh_espnow_led. @note Example - ON / OFF. @attention Must be same with set on led_config_message structure.
                uint8_t brightness;          // Brightness value.
                uint16_t temperature;        // White color temperature value (if present).
                uint8_t red;                 // Red color value (if present).
                uint8_t green;               // Green color value (if present).
                uint8_t blue;                // Blue color value (if present).
                ha_led_effect_type_t effect; // Reserved for future development.
            } led_status_message;
            struct // Tertiary structure of zh_espnow_switch node status message.
            {
                ha_on_off_type_t status; // Status of the zh_espnow_switch. @note Example - ON / OFF. @attention Must be same with set on switch_config_message structure.
            } switch_status_message;
        } status_message;
        union // Secondary union of structures of any OTA update messages. @attention Not used in this view. Should be converted to the required tertiary structure.
        {
            struct // Tertiary structure for transfer from ESP-NOW node to ESP-NOW gateway system information for OTA update initialization.
            {
                char app_name[32];    // Firmware application name.
                char app_version[32]; // Firmware application version.
            } espnow_ota_data;
            struct // Tertiary structure for transfer from ESP-NOW gateway to ESP-NOW node OTA update data.
            {
                uint16_t part;     // System counter for the number of new firmware sent parts.
                uint8_t data_len;  // Size of sent data @note Except for the last part, the data is transmitted in 200 bytes part size.
                uint8_t data[200]; // Sent data.
            } espnow_ota_message;
        } ota_message;
    } payload_data;

} __attribute__((packed)) zh_espnow_data_t;