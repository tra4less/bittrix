#include "mcp.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

static const char* TAG = "MCP";

struct mcp_t {
    char* coin_id;
};

/**
 * @brief 内部函数：HTTPS GET 并返回 cJSON 对象
 */
static cJSON* http_get_json(const char* url)
{
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));
    config.url = url;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (esp_http_client_perform(client) != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET failed: %s", url);
        esp_http_client_cleanup(client);
        return nullptr;
    }

    int content_len = esp_http_client_get_content_length(client);
    if (content_len <= 0) {
        ESP_LOGE(TAG, "Empty response");
        esp_http_client_cleanup(client);
        return nullptr;
    }

    char* buffer = (char*)malloc(content_len + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Malloc failed");
        esp_http_client_cleanup(client);
        return nullptr;
    }

    int read_len = esp_http_client_read_response(client, buffer, content_len);
    if (read_len <= 0) {
        ESP_LOGE(TAG, "Failed to read response");
        free(buffer);
        esp_http_client_cleanup(client);
        return nullptr;
    }

    buffer[read_len] = 0;
    cJSON* json = cJSON_Parse(buffer);
    free(buffer);
    esp_http_client_cleanup(client);

    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
    }
    return json;
}

/**
 * @brief 创建 MCP 实例
 */
mcp_t* mcp_create(const char* coin_id)
{
    mcp_t* handle = (mcp_t*)malloc(sizeof(mcp_t));
    handle->coin_id = strdup(coin_id);
    return handle;
}

/**
 * @brief 获取当前价格
 */
bool mcp_get_price(mcp_t* handle, float* price)
{
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.coingecko.com/api/v3/simple/price?ids=%s&vs_currencies=usd",
             handle->coin_id);

    cJSON* json = http_get_json(url);
    if (!json) return false;

    cJSON* coin_obj = cJSON_GetObjectItem(json, handle->coin_id);
    if (!coin_obj) { cJSON_Delete(json); return false; }

    cJSON* usd = cJSON_GetObjectItem(coin_obj, "usd");
    if (!usd) { cJSON_Delete(json); return false; }

    *price = (float)usd->valuedouble;
    cJSON_Delete(json);
    return true;
}

/**
 * @brief 获取历史价格曲线
 */
bool mcp_get_history(mcp_t* handle, int days, float** prices, size_t* count)
{
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.coingecko.com/api/v3/coins/%s/market_chart?vs_currency=usd&days=%d&interval=hourly",
             handle->coin_id, days);

    cJSON* json = http_get_json(url);
    if (!json) return false;

    cJSON* arr = cJSON_GetObjectItem(json, "prices");
    if (!arr || !cJSON_IsArray(arr)) { cJSON_Delete(json); return false; }

    size_t n = cJSON_GetArraySize(arr);
    *prices = (float*)malloc(sizeof(float) * n);
    *count = n;

    for (size_t i = 0; i < n; i++) {
        cJSON* item = cJSON_GetArrayItem(arr, i);
        if (cJSON_IsArray(item) && cJSON_GetArraySize(item) >= 2) {
            cJSON* price_val = cJSON_GetArrayItem(item, 1);
            (*prices)[i] = (float)price_val->valuedouble;
        } else {
            (*prices)[i] = 0.0f;
        }
    }

    cJSON_Delete(json);
    return true;
}

/**
 * @brief 释放 MCP 实例
 */
void mcp_destroy(mcp_t* handle)
{
    if (handle) {
        if (handle->coin_id) free(handle->coin_id);
        free(handle);
    }
}
