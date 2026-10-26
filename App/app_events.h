#ifndef __APP_EVENTS_H
#define __APP_EVENTS_H

#include "main.h"
#include <stdbool.h>

/**
 * @brief 应用层事件类型
 *
 * 约定：所有中断服务函数只负责投递事件，不做任何 IO / 延时 / 状态判断。
 */
typedef enum
{
    APP_EVENT_NONE = 0,

    APP_EVENT_KEYPAD,       /* 触摸键盘 IRQ（读键在主线程序列化执行） */
    APP_EVENT_FINGERPRINT,  /* 指纹模块触摸 IRQ                      */
    APP_EVENT_BODY,         /* 人体感应模块                          */
    APP_EVENT_DOOR_SENSE,   /* 霍尔/门磁（预留）                      */
} AppEventType;

typedef struct
{
    AppEventType type;
    uint32_t     tick;      /* 事件产生时刻（HAL_GetTick） */
} AppEvent;

/** 初始化事件队列（可重复调用，会把队列清空） */
void AppEvents_Init(void);

/** 中断上下文安全：投递一个事件；队列满时丢弃并返回 false */
bool AppEvents_PushFromISR(AppEventType type);

/** 主循环上下文：取出一个事件；队列为空返回 false */
bool AppEvents_Pop(AppEvent *event);

/** 丢弃所有未处理事件 */
void AppEvents_Flush(void);

#endif /* __APP_EVENTS_H */
