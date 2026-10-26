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

/*
 * 故障记录：放在 SRAM2 末尾（链接脚本没有使用这段 RAM），
 * 系统复位（看门狗/软件复位）后内容会保留，方便下次启动打印上次故障类型。
 * 上电冷启动时内容随机，用 magic 校验。
 */
#define APP_FAULT_RECORD_ADDR  0x2001FFF0UL
#define APP_FAULT_RECORD_MAGIC 0x5A5AA5A5UL

typedef struct
{
    volatile uint32_t magic;
    volatile uint32_t code;
    volatile uint32_t code_inv;
} AppFaultRecord;

static inline volatile AppFaultRecord *App_GetFaultRecord(void)
{
    return (volatile AppFaultRecord *)APP_FAULT_RECORD_ADDR;
}

/* 故障码 */
#define APP_FAULT_NONE        0U
#define APP_FAULT_HARDFAULT   1U
#define APP_FAULT_MEMMANAGE   2U
#define APP_FAULT_BUSFAULT    3U
#define APP_FAULT_USAGEFAULT  4U
#define APP_FAULT_NMI         5U
#define APP_FAULT_ERRORHANDLE 6U

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
