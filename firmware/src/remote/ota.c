#include "ota.h"
#include "esp_netif.h"
#include "ota_http.h"
#include <esp_netif_types.h>

static const char *TAG = "PUBMOTE-OTA";

static ota_state_t ota_state = {.in_progress = false,
                                .total_size = 0,
                                .received_size = 0,
                                .progress = 0,
                                .ota_handle = 0,
                                .update_partition = NULL};

// HTML for OTA Update UI
static const char *ota_html = "<html lang=\"en\">\n"
                              "\n"
                              "<head>\n"
                              "  <meta charset=\"UTF-8\">\n"
                              "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                              "  <title>Pubmote OTA</title>\n"
                              "  <style>\n"
                              "    body {\n"
                              "      font-family: 'Arial', sans-serif;\n"
                              "      margin: 0;\n"
                              "      padding: 20px;\n"
                              "      color: #333;\n"
                              "      background-color: #f4f4f4;\n"
                              "    }\n"
                              "\n"
                              "    .container {\n"
                              "      max-width: 500px;\n"
                              "      margin: 0 auto;\n"
                              "      background: white;\n"
                              "      padding: 20px;\n"
                              "      border-radius: 8px;\n"
                              "      box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);\n"
                              "    }\n"
                              "\n"
                              "    h1 {\n"
                              "      text-align: center;\n"
                              "      color: #2c3e50;\n"
                              "    }\n"
                              "\n"
                              "    .form-container {\n"
                              "      margin-top: 20px;\n"
                              "    }\n"
                              "\n"
                              "    .file-input {\n"
                              "      width: 100%;\n"
                              "      padding: 10px;\n"
                              "      margin-bottom: 15px;\n"
                              "      border: 1px solid #ddd;\n"
                              "      border-radius: 4px;\n"
                              "    }\n"
                              "\n"
                              "    .submit-btn {\n"
                              "      display: block;\n"
                              "      width: 100%;\n"
                              "      padding: 10px;\n"
                              "      background-color: #3498db;\n"
                              "      color: white;\n"
                              "      border: none;\n"
                              "      border-radius: 4px;\n"
                              "      cursor: pointer;\n"
                              "      font-size: 16px;\n"
                              "      transition: background-color 0.3s;\n"
                              "    }\n"
                              "\n"
                              "    .submit-btn:hover {\n"
                              "      background-color: #2980b9;\n"
                              "    }\n"
                              "\n"
                              "    .submit-btn:disabled {\n"
                              "      background-color: #95a5a6;\n"
                              "      cursor: not-allowed;\n"
                              "    }\n"
                              "\n"
                              "    .progress-container {\n"
                              "      margin-top: 20px;\n"
                              "      display: none;\n"
                              "    }\n"
                              "\n"
                              "    .progress-bar {\n"
                              "      height: 20px;\n"
                              "      background-color: #ecf0f1;\n"
                              "      border-radius: 10px;\n"
                              "      overflow: hidden;\n"
                              "      position: relative;\n"
                              "    }\n"
                              "\n"
                              "    .progress {\n"
                              "      width: 0%;\n"
                              "      height: 100%;\n"
                              "      background-color: #2ecc71;\n"
                              "      transition: width 0.3s ease;\n"
                              "    }\n"
                              "\n"
                              "    .progress-text {\n"
                              "      text-align: center;\n"
                              "      margin-top: 5px;\n"
                              "      font-size: 14px;\n"
                              "    }\n"
                              "\n"
                              "    .status {\n"
                              "      margin-top: 20px;\n"
                              "      text-align: center;\n"
                              "      font-weight: bold;\n"
                              "      display: none;\n"
                              "    }\n"
                              "\n"
                              "    .success {\n"
                              "      color: #27ae60;\n"
                              "    }\n"
                              "\n"
                              "    .error {\n"
                              "      color: #e74c3c;\n"
                              "    }\n"
                              "\n"
                              "    .info {\n"
                              "      margin-top: 20px;\n"
                              "      font-size: 14px;\n"
                              "      color: #7f8c8d;\n"
                              "      text-align: center;\n"
                              "    }\n"
                              "  </style>\n"
                              "</head>\n"
                              "\n"
                              "<body>\n"
                              "  <div class=\"container\">\n"
                              "    <h1>Pubmote OTA</h1>\n"
                              "    <div class=\"form-container\">\n"
                              "      <form id=\"upload-form\" enctype=\"multipart/form-data\"> <input type=\"file\" "
                              "id=\"firmware\" name=\"firmware\"\n"
                              "          accept=\".bin\" class=\"file-input\"> <button type=\"submit\" "
                              "id=\"submit-btn\" class=\"submit-btn\">Update\n"
                              "          Firmware</button> </form>\n"
                              "    </div>\n"
                              "    <div class=\"progress-container\" id=\"progress-container\">\n"
                              "      <div class=\"progress-bar\">\n"
                              "        <div class=\"progress\" id=\"progress\"></div>\n"
                              "      </div>\n"
                              "      <div class=\"progress-text\" id=\"progress-text\">0%</div>\n"
                              "    </div>\n"
                              "    <div class=\"status\" id=\"status\"></div>\n"
                              "    <div class=\"info\">\n"
                              "      <p>Current Firmware: <span id=\"version\">ESP32-S3 Device</span></p>\n"
                              "      <p>Connected to: <span id=\"device-name\">ESP32-S3 Device</span></p>\n"
                              "    </div>\n"
                              "  </div>\n"
                              "  <script>document.addEventListener(\"DOMContentLoaded\", function () {\n"
                              "      const form = document.getElementById(\"upload-form\");\n"
                              "      const submitBtn = document.getElementById(\"submit-btn\");\n"
                              "      const progressContainer = document.getElementById(\"progress-container\");\n"
                              "      const progressBar = document.getElementById(\"progress\");\n"
                              "      const progressText = document.getElementById(\"progress-text\");\n"
                              "      const statusDiv = document.getElementById(\"status\");\n"
                              "      form.addEventListener(\"submit\", function (e) {\n"
                              "        e.preventDefault();\n"
                              "        const fileInput = document.getElementById(\"firmware\");\n"
                              "        const file = fileInput.files[0];\n"
                              "        if (!file) {\n"
                              "          showStatus(\"Please select a firmware file (.bin)\", \"error\");\n"
                              "          return;\n"
                              "        }\n"
                              "        if (!file.name.endsWith(\".bin\")) {\n"
                              "          showStatus(\"Please select a valid firmware file (.bin)\", \"error\");\n"
                              "          return;\n"
                              "        }\n"
                              "        const formData = new FormData();\n"
                              "        formData.append(\"firmware\", file);\n"
                              "        // Disable the submit button\n"
                              "        submitBtn.disabled = true;\n"
                              "        submitBtn.textContent = \"Uploading...\";\n"
                              "        // Show progress bar\n"
                              "        progressContainer.style.display = \"block\";\n"
                              "        statusDiv.style.display = \"none\";\n"
                              "        const xhr = new XMLHttpRequest();\n"
                              "        xhr.open(\"POST\", \"/update\");\n"
                              "        xhr.upload.onprogress = function (e) {\n"
                              "          if (e.lengthComputable) {\n"
                              "            const percentComplete = Math.floor((e.loaded / e.total) * 100);\n"
                              "            progressBar.style.width = percentComplete + \"%\";\n"
                              "            progressText.textContent = percentComplete + \"%\";\n"
                              "          }\n"
                              "        };\n"
                              "        xhr.onload = function () {\n"
                              "          if (xhr.status === 200) {\n"
                              "            showStatus(\"Update successful! Device is restarting...\", \"success\");\n"
                              "            setTimeout(function () {\n"
                              "              window.location.reload();\n"
                              "            }, 5000);\n"
                              "          } else {\n"
                              "            showStatus(\"Update failed: \" + xhr.responseText, \"error\");\n"
                              "            submitBtn.disabled = false;\n"
                              "            submitBtn.textContent = \"Update Firmware\";\n"
                              "          }\n"
                              "        };\n"
                              "        xhr.onerror = function () {\n"
                              "          showStatus(\"Connection error. Please try again.\", \"error\");\n"
                              "          submitBtn.disabled = false;\n"
                              "          submitBtn.textContent = \"Update Firmware\";\n"
                              "        };\n"
                              "        xhr.send(formData);\n"
                              "      });\n"
                              "      function showStatus(message, type) {\n"
                              "        statusDiv.textContent = message;\n"
                              "        statusDiv.className = \"status \" + type;\n"
                              "        statusDiv.style.display = \"block\";\n"
                              "      }\n"
                              "    });\n"
                              "  </script>\n"
                              "</body>\n"
                              "</html>";

// Function to handle the root path and serve the OTA Update UI
static esp_err_t ota_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "OTA Update requested");
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, ota_html, strlen(ota_html));
  return ESP_OK;
}

// Function to handle the firmware upload path
static esp_err_t ota_update_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "OTA Update uploaded");
  char buf[FIRMWARE_BUFFER_SIZE];
  int total_received = 0;
  esp_err_t err;
  int content_length = req->content_len;
  int ret, remaining = req->content_len;

  ESP_LOGI(TAG, "Update request received, size: %d bytes", content_length);

  // Check if we already have an OTA in progress
  if (ota_state.in_progress) {
    const char *error_msg = "Another update is already in progress";
    httpd_resp_set_status(req, "409 Conflict");
    httpd_resp_send(req, error_msg, strlen(error_msg));
    return ESP_FAIL;
  }

  // Get the update partition
  ota_state.update_partition = esp_ota_get_next_update_partition(NULL);
  if (ota_state.update_partition == NULL) {
    const char *error_msg = "Failed to get update partition";
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_send(req, error_msg, strlen(error_msg));
    return ESP_FAIL;
  }

  // ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", ota_state.update_partition->subtype,
  //          ota_state.update_partition->address);

  // Begin OTA
  err = esp_ota_begin(ota_state.update_partition, OTA_SIZE_UNKNOWN, &ota_state.ota_handle);
  if (err != ESP_OK) {
    const char *error_msg = "Failed to begin OTA update";
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_send(req, error_msg, strlen(error_msg));
    return ESP_FAIL;
  }

  // Set OTA state
  ota_state.in_progress = true;
  ota_state.total_size = content_length;
  ota_state.received_size = 0;
  ota_state.progress = 0;

  // Process the uploaded firmware in chunks
  while (remaining > 0) {
    // Receive data
    if ((ret = httpd_req_recv(req, buf, (remaining < FIRMWARE_BUFFER_SIZE ? remaining : FIRMWARE_BUFFER_SIZE) <= 0))) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        // Retry receiving if timeout occurred
        continue;
      }

      // Handle error
      ESP_LOGE(TAG, "Failed to receive firmware data: %d", ret);
      esp_ota_abort(ota_state.ota_handle);
      ota_state.in_progress = false;

      const char *error_msg = "Failed to receive firmware data";
      httpd_resp_set_status(req, "500 Internal Server Error");
      httpd_resp_send(req, error_msg, strlen(error_msg));

      return ESP_FAIL;
    }

    // Write to flash
    err = esp_ota_write(ota_state.ota_handle, (const void *)buf, ret);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error writing to flash: %s", esp_err_to_name(err));
      esp_ota_abort(ota_state.ota_handle);
      ota_state.in_progress = false;

      const char *error_msg = "Error writing firmware to flash";
      httpd_resp_set_status(req, "500 Internal Server Error");
      httpd_resp_send(req, error_msg, strlen(error_msg));

      return ESP_FAIL;
    }

    // Update progress
    remaining -= ret;
    total_received += ret;
    ota_state.received_size = total_received;
    ota_state.progress = (total_received * 100) / content_length;

    ESP_LOGI(TAG, "OTA Progress: %d%%", ota_state.progress);
  }

  // End OTA
  err = esp_ota_end(ota_state.ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error ending OTA update: %s", esp_err_to_name(err));
    ota_state.in_progress = false;

    const char *error_msg = "Error finalizing firmware update";
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_send(req, error_msg, strlen(error_msg));

    return ESP_FAIL;
  }

  // Set boot partition
  err = esp_ota_set_boot_partition(ota_state.update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error setting boot partition: %s", esp_err_to_name(err));
    ota_state.in_progress = false;

    const char *error_msg = "Error setting boot partition";
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_send(req, error_msg, strlen(error_msg));

    return ESP_FAIL;
  }

  // Update successful!
  ota_state.in_progress = false;
  ESP_LOGI(TAG, "OTA update successful! Rebooting...");

  const char *success_msg = "Firmware update successful! Device will restart in a few seconds.";
  httpd_resp_set_status(req, "200 OK");
  httpd_resp_send(req, success_msg, strlen(success_msg));

  // Schedule restart task
  vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds delay before restart
  esp_restart();

  return ESP_OK;
}

// Register HTTP server handlers for OTA
static void ota_register_handlers(httpd_handle_t server) {
  // Handler for OTA UI webpage
  httpd_uri_t ota_get = {.uri = "/update", .method = HTTP_GET, .handler = ota_get_handler, .user_ctx = NULL};
  httpd_register_uri_handler(server, &ota_get);

  // Handler for firmware upload endpoint
  httpd_uri_t ota_post = {.uri = "/update", .method = HTTP_POST, .handler = ota_update_handler, .user_ctx = NULL};
  httpd_register_uri_handler(server, &ota_post);

  ESP_LOGI(TAG, "OTA handlers registered successfully");
}

// Get the current OTA progress (0-100)
int ota_get_progress(void) {
  return ota_state.progress;
}

// Check if OTA is in progress
bool ota_is_updating(void) {
  return ota_state.in_progress;
}

static void print_ip_info(void) {
  esp_netif_t *netif = NULL;
  esp_netif_ip_info_t ip_info;

  // For Station mode
  netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif) {
    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
    ESP_LOGI("WIFI", "STA IP: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI("WIFI", "STA MASK: " IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI("WIFI", "STA GW: " IPSTR, IP2STR(&ip_info.gw));
  }

  // For AP mode
  netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (netif) {
    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
    ESP_LOGI("WIFI", "AP IP: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI("WIFI", "AP MASK: " IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI("WIFI", "AP GW: " IPSTR, IP2STR(&ip_info.gw));
  }
}

static httpd_handle_t server;

void init_ota() {
  // Start the web server
  server = start_webserver();
  if (server) {
    ota_register_handlers(server);
    // Get current firmware version (optional)
    const esp_app_desc_t *app_desc = esp_app_get_description();
    ESP_LOGI(TAG, "Running firmware version: %s", app_desc->version);
    ESP_LOGI(TAG, "OTA ready! Open http://[device-ip]/update in your browser");
    // print_ip_info();
  }
}

void teardown_ota() {
  if (server) {
    stop_webserver(server);
    server = NULL;
  }
}