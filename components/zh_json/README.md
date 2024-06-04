# ESP32 ESP-IDF and ESP8266 RTOS SDK component for json

Currently only a simple json is being created. Will be updated as needed.

## Tested on

1. ESP8266 RTOS_SDK v3.4
2. ESP32 ESP-IDF v5.2

## [Function description](http://zh-json.zh.com.ru)

## Using

In an existing project, run the following command to install the components:

```text
cd ../your_project/components
git clone https://github.com/aZholtikov/zh_json.git
```

In the application, add the component:

```c
#include "zh_json.h"
```

## Example

Create json:

```c
#include "stdio.h"
#include "zh_json.h"

void app_main(void)
{
    zh_json_t json = {0};
    zh_json_init(&json);
    zh_json_add(&json, "Name 1", "Hello world!");
    zh_json_add(&json, "Name 2", "true");
    zh_json_add(&json, "Name 3", "255");
    zh_json_add(&json, "Name 4", "123,456");
    char buffer[128] = {0};
    zh_json_create(&json, buffer);
    printf("Json is: %s\n", buffer);
    zh_json_free(&json);
}
```

Any [feedback](mailto:github@azholtikov.ru) will be gladly accepted.
