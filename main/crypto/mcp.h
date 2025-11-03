#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief MCP 对象
 */
typedef struct mcp_t mcp_t;

/**
 * @brief 创建一个 MCP 实例（基于 CoinGecko）
 */
mcp_t* mcp_create(const char* coin_id);

/**
 * @brief 获取当前价格
 */
bool mcp_get_price(mcp_t* handle, float* price);

/**
 * @brief 获取历史价格曲线
 */
bool mcp_get_history(mcp_t* handle, int days, float** prices, size_t* count);

/**
 * @brief 释放 MCP 实例
 */
void mcp_destroy(mcp_t* handle);

#ifdef __cplusplus
} // extern "C"
#endif