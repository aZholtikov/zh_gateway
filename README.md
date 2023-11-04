# ESP-NOW Gateway

Gateway for data exchange between ESP-NOW devices and MQTT broker via WiFi/LAN.

## Features

1. Automatically adds gateway configuration to Home Assistan via MQTT discovery as a binary_sensor.
2. Automatically adds supported devices configurations to Home Assistan via MQTT discovery.
3. Update firmware from HTTPS server via OTA.
4. Update firmware of supported devices from HTTPS server via ESP-NOW.

## Notes

1. For initial settings use "menuconfig -> ZH Gateway Configuration".
2. To restart the gateway, send the "restart" command to the root topic of the gateway (example - "homeassistant/gateway/70-03-9F-44-BE-F7").
3. To update the gateway firmware, send the "update" command to the root topic of the gateway (example - "homeassistant/gateway/70-03-9F-44-BE-F7"). The update path should be like as "https://your_server/zh_gateway_esp32.bin". The online status of the update is displayed in the root gateway topic.

## Build and flash

Run the following command to firmware build and flash module:

```text
cd your_projects_folder
bash <(curl -Ls http://git.zh.com.ru/alexey.zholtikov/zh_gateway/raw/branch/main/install.sh)
idf.py menuconfig
idf.py all
idf.py -p (PORT) flash
```

## Tested on

1. WT32-ETH01.
2. ESP32 NodeMCU V3.

## Attention

1. If using a WiFi connection, the WiFi router must be set to channel 1.
2. Only one device can be updated via ESP-NOW at a time. During the device upgrade, there may be delays in the response of others devices on the network.
3. The certificate (certificate.pem) must match the upgrade server!

## Supported devices

1. [zh_espnow_switch](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch)