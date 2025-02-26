#ifndef __OTA_HTTP_H
#define __OTA_HTTP_H
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_wifi.h>

httpd_handle_t start_webserver();

#endif