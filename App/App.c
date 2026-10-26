#include "App.h"
#include "app_events.h"
#include "lock_app.h"
#include "stepper_motor.h"
#include "Keyboard.h"
#include "lcd.h"
#include "Fingerprint.h"

extern TIM_HandleTypeDef htim6;
extern IWDG_HandleTypeDef hiwdg;

void App_Init(void)
{
    AppEvents_Init();

    /* 指示灯默认熄灭（低电平点亮） */
    HAL_GPIO_WritePin(GPIOA, Led1_Pin | Led2_Pin, GPIO_PIN_SET);

    StepperMotor_Init(&htim6);
    Keyboard_Init();
    LCD_Init();

    LockApp_Init();
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
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case KEYBOARD_IRQ_Pin:
            (void)AppEvents_PushFromISR(APP_EVENT_KEYPAD);
            break;

        case FP_IRQ_Pin:
            (void)AppEvents_PushFromISR(APP_EVENT_FINGERPRINT);
            break;

        case Hall_IRQ_Pin:
            (void)AppEvents_PushFromISR(APP_EVENT_DOOR_SENSE);
            break;

        case GPIO_PIN_0: /* 人体感应模块 */
#ifdef BODY_WAKEUP
            (void)AppEvents_PushFromISR(APP_EVENT_BODY);
#endif
            break;

        default:
            break;
    }
}
