#include "multi_timer.h"

static volatile uint32_t timer_counter = 0;
static volatile uint8_t timer_enabled = 0;
static uint32_t target_count_1 = 0;
static uint32_t target_count_2 = 0;
static TimerCallback callback_1s = NULL;
static TimerCallback callback_2s = NULL;
extern TIM_HandleTypeDef htim7;
void MultiTimer_Init(void)
{
    timer_counter = 0;
    timer_enabled = 0;
    target_count_1 = 0;
    target_count_2 = 0;
    callback_1s = NULL;
    callback_2s = NULL;
    
    __HAL_RCC_TIM7_CLK_ENABLE();
    
    htim7.Instance = TIM7;
    htim7.Init.Prescaler = 8399;
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim7.Init.Period = 999;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim7);
    
    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

void MultiTimer_Start(uint32_t seconds_1, TimerCallback func_1, 
                     uint32_t seconds_2, TimerCallback func_2)
{
    timer_counter = 0;
    target_count_1 = seconds_1 * 10;
    target_count_2 = seconds_2 * 10;
    callback_1s = func_1;
    callback_2s = func_2;
    timer_enabled = 1;
    
    __HAL_TIM_SET_COUNTER(&htim7, 0);
    HAL_TIM_Base_Start_IT(&htim7);
}

void MultiTimer_Stop(void)
{
    timer_enabled = 0;
    timer_counter = 0;
    HAL_TIM_Base_Stop_IT(&htim7);
}

uint8_t MultiTimer_IsRunning(void)
{
    return timer_enabled;
}

void MultiTimer_IRQ_Handler(void)
{
    if (timer_enabled)
    {
        timer_counter++;
        
        // 第一个时间点回调（seconds_1秒后执行）
        if (timer_counter == target_count_1 && callback_1s != NULL) {
            callback_1s();
        }
        
        // 第二个时间点回调（seconds_2秒后执行）
        if (timer_counter == target_count_2 && callback_2s != NULL) {
            callback_2s();
            MultiTimer_Stop();
        }
    }
}