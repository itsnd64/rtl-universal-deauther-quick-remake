#pragma once

#define delay(x) vTaskDelay(x)

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5] // well i might have used esp32 a bit too much
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"