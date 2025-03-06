#include "ota_http.h"
#include "esp_log.h"

static const char *TAG = "PUBMOTE-OTA_HTTP";

/* Simple HTTP server handler for root path */
static esp_err_t root_get_handler(httpd_req_t *req) {
  const char *response = "<html><head><title>ESP32-S3 Device</title></head>"
                         "<body><h1>ESP32-S3 Device</h1>"
                         "<p>Welcome to your ESP32-S3 device.</p>"
                         "<p><a href='/update'>Go to OTA Update page</a></p>"
                         "</body></html>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

/* Start the HTTP server */
httpd_handle_t start_webserver() {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;

  // Start the HTTP server
  if (httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "Server started");

    // Register URI handlers
    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL};
    httpd_register_uri_handler(server, &root);

    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

void stop_webserver(httpd_handle_t server) {
  // Stop the HTTP server
  httpd_stop(server);
  ESP_LOGI(TAG, "Server stopped");
}