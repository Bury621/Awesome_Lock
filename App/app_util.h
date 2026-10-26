#ifndef __APP_UTIL_H
#define __APP_UTIL_H

#include "main.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/** 数组元素个数 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/** 把毫秒换算成 HAL tick（HAL_GetTick 固定为 1ms） */
#define APP_MS_TO_TICKS(ms) ((uint32_t)(ms))

/**
 * @brief 进入临界区（保存并关闭全局中断）
 * @return 进入前的 PRIMASK，用于退出时恢复
 */
static inline uint32_t App_EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/** @brief 退出临界区，恢复进入前的中断状态 */
static inline void App_ExitCritical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/**
 * @brief 判断某个截止时间是否已到（自动处理 HAL_GetTick 回绕）
 * @param deadline 绝对截止 tick
 */
static inline bool App_TimeReached(uint32_t deadline)
{
    return ((int32_t)(HAL_GetTick() - deadline) >= 0);
}

/** @brief 距离截止时间还剩多少 ms，已过期返回 0 */
static inline uint32_t App_TimeRemaining(uint32_t deadline)
{
    int32_t remain = (int32_t)(deadline - HAL_GetTick());
    return (remain > 0) ? (uint32_t)remain : 0U;
}

#endif /* __APP_UTIL_H */
