#include "App.h"
#include "app_events.h"
#include "lock_app.h"
#include "stepper_motor.h"
#include "Keyboard.h"
#include "lcd.h"
#include "Fingerprint.h"
#include "Debug.h"

extern TIM_HandleTypeDef htim6;
extern IWDG_HandleTypeDef hiwdg;

void App_Init(void)
{
    AppEvents_Init();

    /* 调试串口（USART1，9600）：如果反复重启，看最后停在哪一步 */
    uart_printf("APP: init start\r\n");

    /* 指示灯默认熄灭（低电平点亮） */
    HAL_GPIO_WritePin(GPIOA, Led1_Pin | Led2_Pin, GPIO_PIN_SET);

    StepperMotor_Init(&htim6);
    uart_printf("APP: motor ok\r\n");

    Keyboard_Init();
    uart_printf("APP: keypad ok\r\n");

    LCD_Init();
    uart_printf("APP: lcd ok (fault=%u)\r\n", (unsigned)LCD_IsFaulted());
    (void)HAL_IWDG_Refresh(&hiwdg);

    LockApp_Init();
    uart_printf("APP: lock ready\r\n");

    (void)HAL_IWDG_Refresh(&hiwdg);
}

void App_Running(void)
{
    AppEvent event;

    /* 1. 处理中断投递的事件（按键、指纹触摸、人体感应） */
    while (AppEvents_Pop(&event))
    {
        LockApp_HandleEvent(&event);
    }

    /* 2. 指纹通信状态机（响应帧、超时、灯光队列） */
    FPC_Poll();

    /* 3. 业务轮询（按键去抖、开关门、倒计时、休眠） */
    LockApp_Poll();

    /* 4. 主循环存活才喂狗；任何任务卡死都会在约 10s 后自动复位 */
    (void)HAL_IWDG_Refresh(&hiwdg);
}

/* TIM6 只用于步进电机节拍，中断里只做 GPIO 翻转和计数 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    StepperMotor_TIM6_Callback(htim);
}

/*
 * 外部中断只投递事件，绝不在这里读 I2C、等 UART、刷屏或延时。
 * 这些操作全部搬到主循环，避免中断长时间占用导致 SysTick/看门狗被饿死。
 *
 * 每个来源再加一个最小间隔限制：即使某个输入脚悬空/受干扰产生高频边沿，
 * 也不会把事件队列和主循环淹没。
 */
static uint32_t s_last_key_tick;
static uint32_t s_last_fp_tick;
static uint32_t s_last_body_tick;
static uint32_t s_last_door_tick;

static bool edge_allowed(uint32_t *last_tick, uint32_t min_interval_ms)
{
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - *last_tick) < min_interval_ms)
    {
        return false;
    }
    *last_tick = now;
    return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case KEYBOARD_IRQ_Pin:
            if (edge_allowed(&s_last_key_tick, 5U))
            {
                (void)AppEvents_PushFromISR(APP_EVENT_KEYPAD);
            }
            break;

        case FP_IRQ_Pin:
            if (edge_allowed(&s_last_fp_tick, 20U))
            {
                (void)AppEvents_PushFromISR(APP_EVENT_FINGERPRINT);
            }
            break;

        case Hall_IRQ_Pin:
            if (edge_allowed(&s_last_door_tick, 50U))
            {
                (void)AppEvents_PushFromISR(APP_EVENT_DOOR_SENSE);
            }
            break;

        case GPIO_PIN_0: /* 人体感应模块 */
#ifdef BODY_WAKEUP
            if (edge_allowed(&s_last_body_tick, 50U))
            {
                (void)AppEvents_PushFromISR(APP_EVENT_BODY);
            }
#endif
            break;

        default:
            break;
    }
}
