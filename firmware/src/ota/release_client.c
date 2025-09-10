#include "release_client.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include <esp_err.h>
#include <esp_log.h>

static const char *TAG = "PUBMOTE-GITHUB_FETCH";

#define GITHUB_API "https://api.github.com"
#define GITHUB_REPO "contactsimonwilson/pubremote"
#define MAX_HTTP_OUTPUT_BUFFER 8192

// Escaped version of release_query.gql
static const char *GITHUB_ASSET_QUERY =
    "{"
    "\"query\": \"{"
    "repository(owner: \\\"contactsimonwilson\\\", name: \\\"PubRemote\\\") {"
    "stable: latestRelease {"
    "name "
    "tagName "
    "assets: releaseAssets(first: 50) {"
    "nodes {"
    "name "
    "downloadUrl "
    "}"
    "}"
    "}"
    "prerelease: releases(first: 2, orderBy: {field: CREATED_AT, direction: DESC}) {"
    "nodes {"
    "name "
    "tagName "
    "isPrerelease "
    "assets: releaseAssets(first: 50) {"
    "nodes {"
    "name "
    "downloadUrl "
    "}"
    "}"
    "}"
    "}"
    "}"
    "}\""
    "}";

// Buffer to store HTTP response
static char *http_response_buffer = NULL;
static int http_response_len = 0;

// HTTP event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

    // Allocate buffer for response if not already allocated (use PSRAM)
    if (http_response_buffer == NULL) {
      http_response_buffer = heap_caps_malloc(32768, MALLOC_CAP_SPIRAM);
      if (http_response_buffer == NULL) {
        // Fallback to internal memory if PSRAM fails
        http_response_buffer = malloc(32768);
      }
      http_response_len = 0;

      if (http_response_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate HTTP response buffer");
        return ESP_ERR_NO_MEM;
      }
    }

    // Copy data to buffer if there's space
    if (http_response_len + evt->data_len < 32768 - 1) {
      memcpy(http_response_buffer + http_response_len, evt->data, evt->data_len);
      http_response_len += evt->data_len;
      http_response_buffer[http_response_len] = '\0'; // Null terminate
    }
    else {
      ESP_LOGW(TAG, "HTTP response too large, truncating");
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

/**
 * Fetch asset download URLs for stable, prerelease, and nightly builds
 *
 * @param asset_name The asset name to search for (substring matching)
 * @param result Pointer to structure that will hold all found URLs
 *
 * @return ESP_OK if at least one asset found, ESP_ERR_NOT_FOUND if none found, other error codes on failure
 */
esp_err_t fetch_all_asset_urls(const char *asset_name, github_asset_urls_t *result) {
  if (asset_name == NULL || result == NULL) {
    ESP_LOGE(TAG, "Invalid parameters");
    return ESP_ERR_INVALID_ARG;
  }

  // Initialize result structure
  memset(result, 0, sizeof(github_asset_urls_t));

  ESP_LOGI(TAG, "Searching for asset: '%s' in all release types", asset_name);

  const char *graphql_url = "https://api.github.com/graphql";

  // Reset response buffer
  if (http_response_buffer != NULL) {
    free(http_response_buffer);
    http_response_buffer = NULL;
    http_response_len = 0;
  }

  esp_http_client_config_t config = {
      .url = graphql_url,
      .event_handler = http_event_handler,
      .timeout_ms = 15000,
      .buffer_size = 4096,
      .buffer_size_tx = 4096,
      .transport_type = HTTP_TRANSPORT_OVER_SSL,
      .method = HTTP_METHOD_POST,
      .disable_auto_redirect = false,
      // TODO - fix security
      .skip_cert_common_name_check = true,
      .use_global_ca_store = false,
      .cert_pem = NULL,
      .client_cert_pem = NULL,
      .client_key_pem = NULL,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_header(client, "Accept", "application/json");
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_header(client, "User-Agent", "ESP32-GitHub-Client");
  esp_http_client_set_post_field(client, GITHUB_ASSET_QUERY, strlen(GITHUB_ASSET_QUERY));

  esp_err_t err = esp_http_client_perform(client);
  esp_err_t function_result = ESP_ERR_NOT_FOUND; // Default to not found

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);

    if (status_code == 200 && http_response_buffer != NULL) {
      ESP_LOGI(TAG, "GraphQL response: %d bytes", http_response_len);

      cJSON *json = cJSON_Parse(http_response_buffer);
      if (json != NULL) {
        cJSON *data = cJSON_GetObjectItem(json, "data");
        if (data && cJSON_GetObjectItem(data, "repository")) {
          cJSON *repository = cJSON_GetObjectItem(data, "repository");

          // Search in stable release
          cJSON *stable = cJSON_GetObjectItem(repository, "stable");
          if (stable && !cJSON_IsNull(stable)) {
            cJSON *tag_name = cJSON_GetObjectItem(stable, "tagName");
            cJSON *assets = cJSON_GetObjectItem(stable, "assets");

            ESP_LOGI(TAG, "Searching in stable release: %s",
                     cJSON_IsString(tag_name) ? tag_name->valuestring : "unknown");

            if (assets && cJSON_IsString(tag_name)) {
              cJSON *asset_nodes = cJSON_GetObjectItem(assets, "nodes");
              if (cJSON_IsArray(asset_nodes)) {
                int asset_count = cJSON_GetArraySize(asset_nodes);

                for (int i = 0; i < asset_count; i++) {
                  cJSON *asset = cJSON_GetArrayItem(asset_nodes, i);
                  cJSON *name = cJSON_GetObjectItem(asset, "name");
                  cJSON *download_url = cJSON_GetObjectItem(asset, "downloadUrl");

                  if (cJSON_IsString(name) && cJSON_IsString(download_url)) {
                    if (strstr(name->valuestring, asset_name) != NULL) {
                      ESP_LOGI(TAG, "Found asset in stable: %s", name->valuestring);

                      strncpy(result->stable_url, download_url->valuestring, sizeof(result->stable_url) - 1);
                      result->stable_url[sizeof(result->stable_url) - 1] = '\0';

                      strncpy(result->stable_tag, tag_name->valuestring, sizeof(result->stable_tag) - 1);
                      result->stable_tag[sizeof(result->stable_tag) - 1] = '\0';

                      result->stable_found = true;
                      function_result = ESP_OK;
                      break; // Take first match
                    }
                  }
                }
              }
            }
          }

          // Search in recent releases for prerelease and nightly
          cJSON *recent_releases = cJSON_GetObjectItem(repository, "prerelease");
          if (recent_releases) {
            cJSON *nodes = cJSON_GetObjectItem(recent_releases, "nodes");
            if (cJSON_IsArray(nodes)) {
              for (int i = 0; i < cJSON_GetArraySize(nodes); i++) {
                cJSON *release = cJSON_GetArrayItem(nodes, i);
                cJSON *tag_name = cJSON_GetObjectItem(release, "tagName");
                cJSON *assets = cJSON_GetObjectItem(release, "assets");

                if (!cJSON_IsString(tag_name))
                  continue;

                const char *tag_str = tag_name->valuestring;
                bool is_nightly = strcmp(tag_str, "nightly") == 0;

                if (assets) {
                  cJSON *asset_nodes = cJSON_GetObjectItem(assets, "nodes");
                  if (cJSON_IsArray(asset_nodes)) {
                    int asset_count = cJSON_GetArraySize(asset_nodes);

                    for (int j = 0; j < asset_count; j++) {
                      cJSON *asset = cJSON_GetArrayItem(asset_nodes, j);
                      cJSON *name = cJSON_GetObjectItem(asset, "name");
                      cJSON *download_url = cJSON_GetObjectItem(asset, "downloadUrl");

                      if (cJSON_IsString(name) && cJSON_IsString(download_url)) {
                        if (strstr(name->valuestring, asset_name) != NULL) {

                          // Categorize the release
                          if (is_nightly && !result->nightly_found) {
                            ESP_LOGI(TAG, "Found asset in nightly: %s (%s)", name->valuestring, tag_str);

                            strncpy(result->nightly_url, download_url->valuestring, sizeof(result->nightly_url) - 1);
                            result->nightly_url[sizeof(result->nightly_url) - 1] = '\0';

                            strncpy(result->nightly_tag, tag_str, sizeof(result->nightly_tag) - 1);
                            result->nightly_tag[sizeof(result->nightly_tag) - 1] = '\0';

                            result->nightly_found = true;
                            function_result = ESP_OK;
                          }
                          else if (!is_nightly && !result->prerelease_found) {
                            ESP_LOGI(TAG, "Found asset in prerelease: %s (%s)", name->valuestring, tag_str);

                            strncpy(result->prerelease_url, download_url->valuestring,
                                    sizeof(result->prerelease_url) - 1);
                            result->prerelease_url[sizeof(result->prerelease_url) - 1] = '\0';

                            strncpy(result->prerelease_tag, tag_str, sizeof(result->prerelease_tag) - 1);
                            result->prerelease_tag[sizeof(result->prerelease_tag) - 1] = '\0';

                            result->prerelease_found = true;
                            function_result = ESP_OK;
                          }

                          // Break inner loop if we found the asset in this release
                          break;
                        }
                      }
                    }
                  }
                }

                // Break if we found both prerelease and nightly
                if (result->prerelease_found && result->nightly_found) {
                  break;
                }
              }
            }
          }
        }

        cJSON_Delete(json);
      }
      else {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        function_result = ESP_ERR_INVALID_RESPONSE;
      }
    }
    else {
      ESP_LOGE(TAG, "HTTP request failed with status: %d", status_code);
      function_result = ESP_ERR_HTTP_CONNECT;
    }
  }
  else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    function_result = err;
  }

  // Cleanup
  esp_http_client_cleanup(client);
  if (http_response_buffer != NULL) {
    free(http_response_buffer);
    http_response_buffer = NULL;
  }

  // Log results
  ESP_LOGI(TAG, "=== SEARCH RESULTS FOR '%s' ===", asset_name);
  if (result->stable_found) {
    ESP_LOGI(TAG, "Stable: %s -> %s", result->stable_tag, result->stable_url);
  }
  else {
    ESP_LOGI(TAG, "Stable: Not found");
  }

  if (result->prerelease_found) {
    ESP_LOGI(TAG, "Prerelease: %s -> %s", result->prerelease_tag, result->prerelease_url);
  }
  else {
    ESP_LOGI(TAG, "Prerelease: Not found");
  }

  if (result->nightly_found) {
    ESP_LOGI(TAG, "Nightly: %s -> %s", result->nightly_tag, result->nightly_url);
  }
  else {
    ESP_LOGI(TAG, "Nightly: Not found");
  }

  if (function_result == ESP_ERR_NOT_FOUND) {
    ESP_LOGW(TAG, "Asset '%s' not found in any release type", asset_name);
  }

  return function_result;
}

/**
 * Parse a version string of the form "vX.Y.Z" into its components
 *
 * @param version_str The version string to parse (e.g., "v1.2.3")
 * @param result Pointer to structure that will hold the parsed version
 */
firmware_version_t parse_version_string(const char *version_str) {
  firmware_version_t result = {0, 0, 0};

  if (version_str == NULL) {
    ESP_LOGE(TAG, "Invalid parameters to parse_version_string");
    return result;
  }
  if (version_str[0] == 'v' || version_str[0] == 'V') {
    version_str++; // Skip leading 'v'
  }

  sscanf(version_str, "%d.%d.%d", &result.major, &result.minor, &result.patch);
  ESP_LOGI(TAG, "Parsed version string '%s' into %d.%d.%d", version_str, result.major, result.minor, result.patch);
  return result;
}

/**
 * Compare two firmware versions
 * @param a Pointer to first version
 * @param b Pointer to second version
 * @return true if a > b, false otherwise
 */
bool is_version_greater(const firmware_version_t *a, const firmware_version_t *b) {
  if (a->major != b->major) {
    return a->major > b->major;
  }
  if (a->minor != b->minor) {
    return a->minor > b->minor;
  }
  return a->patch > b->patch;
}