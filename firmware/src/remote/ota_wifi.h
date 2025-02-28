
#ifndef __OTA_WIFI_H
#define __OTA_WIFI_H
#include <esp_err.h>
#include <esp_wifi.h>

#define DEFAULT_WIFI_SSID "Pubmote"
#define DEFAULT_WIFI_PASS "pubmote123"
#define WIFI_CONNECT_MAXIMUM_RETRY 5

void wifi_init_sta();

#endif