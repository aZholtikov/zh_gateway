# ESP-NOW Gateway

Gateway for ESP32 ESP-IDF for data exchange between ESP-NOW devices and MQTT broker via WiFi/LAN.

## Tested on

1. ESP32 ESP-IDF v5.2

## Features

1. Automatically adds gateway configuration to Home Assistan via MQTT discovery as a binary_sensor.
2. Automatically adds supported devices configurations to Home Assistan via MQTT discovery.
3. Update firmware from HTTPS server via OTA.
4. Update firmware of supported devices from HTTPS server via ESP-NOW.
5. Direct or mesh work mode.

## Notes

1. All devices on the network must have the same work mode.
2. ESP-NOW mesh network based on the [zh_network](http://git.zh.com.ru/alexey.zholtikov/zh_network).
3. For initial settings use "menuconfig -> ZH Gateway Configuration".
4. To restart the gateway, send the "restart" command to the root topic of the gateway (example - "homeassistant/gateway/70-03-9F-44-BE-F7").
5. To update the gateway firmware, send the "update" command to the root topic of the gateway (example - "homeassistant/gateway/70-03-9F-44-BE-F7"). The update path should be like as "https://your_server/zh_gateway.bin". The online status of the update is displayed in the root gateway topic.

## Build and flash

Run the following command to firmware build and flash module:

```text
cd your_projects_folder
git clone http://git.zh.com.ru/alexey.zholtikov/zh_gateway.git
cd zh_gateway
git submodule init
git submodule update --remote
idf.py menuconfig
idf.py build
idf.py flash
```

## Attention

1. If using a WiFi connection, the WiFi router must be set to channel 1.
2. Only one device can be updated via ESP-NOW at a time. During the device upgrade, there may be delays in the response of others devices on the network.
3. The certificate (certificate.pem) must match the upgrade server!

## Supported devices

1. [zh_espnow_switch](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch)
2. [zh_espnow_led](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_led)
3. [zh_espnow_sensor](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_sensor)

Any [feedback](mailto:github@azholtikov.ru) will be gladly accepted.
