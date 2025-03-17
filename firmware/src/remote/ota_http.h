#ifndef __OTA_HTTP_H
#define __OTA_HTTP_H
#include <esp_err.h>
#include <esp_http_server.h>

httpd_handle_t start_webserver();
void stop_webserver(httpd_handle_t server);

#endif