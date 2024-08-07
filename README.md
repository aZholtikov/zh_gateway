# ESP-NOW Gateway

Gateway for ESP32 ESP-IDF for data exchange between ESP-NOW devices and MQTT broker via WiFi/LAN.

## Tested on

1. ESP32 ESP-IDF v5.2

## Features

1. LAN or WiFi connection to router.
2. Direct or mesh work mode.
3. Automatically adds gateway configuration to Home Assistan via MQTT discovery as a binary_sensor.
4. Automatically adds supported devices configurations to Home Assistan via MQTT discovery.
5. Update firmware from HTTPS server via OTA (optional).
6. Update firmware of supported devices from HTTPS server via ESP-NOW (optional).
7. Receive time from NTP server and transmit it to supported devices (optional).
8. Transmitting system information to Syslog server (optional).

## Notes

1. All devices on the network must have the same work mode.
2. ESP-NOW mesh network based on the [zh_network](https://github.com/aZholtikov/zh_network).
3. For initial settings use "menuconfig -> ZH Gateway Configuration". After first boot all settings (except work mode) will be stored in NVS memory for prevente change during OTA firmware update. But it is highly recommended to set up the configuration via menuconfig before updating.
4. To restart the gateway, send the "restart" command to the root topic of the gateway (example - "homeassistant/gateway/70-03-9F-44-BE-F7").
5. To update the gateway firmware, send the "update" command to the root topic of the gateway (example - "homeassistant/gateway/70-03-9F-44-BE-F7"). The update path should be like as "https://your_server/zh_gateway_esp32.bin". The online status of the update is displayed in the root gateway topic and Syslog server (if enabled).

## Build and flash

Run the following command to firmware build and flash module:

```text
cd your_projects_folder
git clone --recurse-submodules https://github.com/aZholtikov/zh_gateway.git
cd zh_gateway
idf.py menuconfig
idf.py build
idf.py flash
```

## Attention

1. The program is tested on [LILYGO T-ETH-Lite ESP32](https://github.com/Xinyuan-LilyGO/LilyGO-T-ETH-Series) and [Wireless-Tag WT32-ETH01](https://github.com/ldijkman/WT32-ETH01-LAN-8720-RJ45-). To work on another module it will be necessary change ZH_LAN_MODULE_TYPE and ZH_LAN_MODULE_POWER_PIN (for using LAN connection only). No changes are required when using a WiFi connection.
2. If using a WiFi connection, the WiFi router must be set to the same channel as ESP-NOW.
3. Only one device can be updated via ESP-NOW at a time. During the device upgrade, there may be delays in the response of others devices on the network.
4. The certificate (certificate.pem) must match the upgrade server.

## Supported devices

1. [zh_espnow_switch](https://github.com/aZholtikov/zh_espnow_switch)
2. [zh_espnow_led](https://github.com/aZholtikov/zh_espnow_led)
3. [zh_espnow_sensor](https://github.com/aZholtikov/zh_espnow_sensor)

Any [feedback](mailto:github@azholtikov.ru) will be gladly accepted.
