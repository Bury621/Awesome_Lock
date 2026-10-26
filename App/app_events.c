#include "app_events.h"
#include "app_util.h"

#define APP_EVENT_QUEUE_SIZE 16U

/*
 * 单生产者（各 ISR 之间因 Cortex-M 中断不会互相抢占，等价于单生产者）
 * 单消费者（主循环）的环形队列。
 * head 只由生产者写，tail 只由消费者写，长度用 (head - tail) 计算。
 */
static volatile uint16_t s_head;
static volatile uint16_t s_tail;
static AppEvent          s_queue[APP_EVENT_QUEUE_SIZE];

void AppEvents_Init(void)
{
    uint32_t primask = App_EnterCritical();
    s_head = 0U;
    s_tail = 0U;
    App_ExitCritical(primask);
}

bool AppEvents_PushFromISR(AppEventType type)
{
    bool     pushed = false;
    uint16_t head;
    uint16_t next;

    if (type == APP_EVENT_NONE)
    {
        return false;
    }

    /* 这里用临界区而不是“只写 head”，保证与 Pop 的并发读安全 */
    uint32_t primask = App_EnterCritical();
    head = s_head;
    next = (uint16_t)((head + 1U) % APP_EVENT_QUEUE_SIZE);
    if (next != s_tail)
    {
        s_queue[head].type = type;
        s_queue[head].tick = HAL_GetTick();
        s_head = next;
        pushed = true;
    }
    App_ExitCritical(primask);

    return pushed;
}

bool AppEvents_Pop(AppEvent *event)
{
    bool     popped = false;
    uint16_t tail;

    if (event == NULL)
    {
        return false;
    }

    uint32_t primask = App_EnterCritical();
    tail = s_tail;
    if (tail != s_head)
    {
        *event = s_queue[tail];
        s_tail = (uint16_t)((tail + 1U) % APP_EVENT_QUEUE_SIZE);
        popped = true;
    }
    App_ExitCritical(primask);

    return popped;
}

void AppEvents_Flush(void)
{
    uint32_t primask = App_EnterCritical();
    s_tail = s_head;
    App_ExitCritical(primask);
}
