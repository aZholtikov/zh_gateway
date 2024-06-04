#include "zh_config.h"

char *zh_get_device_type_value_name(zh_device_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        ZH_DEVICE_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_payload_type_value_name(zh_payload_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        ZH_PAYLOAD_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_component_type_value_name(ha_component_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_COMPONENT_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_binary_sensor_device_class_value_name(ha_binary_sensor_device_class_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_BINARY_SENSOR_DEVICE_CLASS
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_cover_device_class_value_name(ha_cover_device_class_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_COVER_DEVICE_CLASS
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_sensor_device_class_value_name(ha_sensor_device_class_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_SENSOR_DEVICE_CLASS
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_switch_device_class_value_name(ha_switch_device_class_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_SWITCH_DEVICE_CLASS
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_on_off_type_value_name(ha_on_off_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_ON_OFF_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_chip_type_value_name(ha_chip_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_CHIP_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_led_type_value_name(ha_led_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_LED_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}

char *zh_get_sensor_type_value_name(ha_sensor_type_t value)
{
    switch (value)
    {
#define DF(_value, _name) \
    case _value:          \
        return _name;
        HA_SENSOR_TYPE
#undef DF
        break;
    default:
        break;
    }
    return "";
}