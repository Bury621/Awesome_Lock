#ifndef __STEPPER_MOTOR_H
#define __STEPPER_MOTOR_H

#include "stm32f4xx_hal.h"
#include "main.h"

/* 电机引脚定义 (PE11-PE14) */
#define MOTOR_IN1_PIN GPIO_PIN_11
#define MOTOR_IN1_PORT GPIOE
#define MOTOR_IN2_PIN GPIO_PIN_12
#define MOTOR_IN2_PORT GPIOE
#define MOTOR_IN3_PIN GPIO_PIN_13
#define MOTOR_IN3_PORT GPIOE
#define MOTOR_IN4_PIN GPIO_PIN_14
#define MOTOR_IN4_PORT GPIOE

/* 28BYJ-48：整圈 4096 步（8 拍半步模式） */
#define STEPS_PER_REVOLUTION 4096U
#define STEP_SEQUENCE_SIZE   8U

/*
 * 步进间隔范围，单位 us。
 * TIM6 被配置为 1MHz 计数（84MHz / 84），因此重载值 = 间隔 - 1。
 */
#define MOTOR_MIN_STEP_US     200U
#define MOTOR_MAX_STEP_US     4000U
#define MOTOR_DEFAULT_STEP_US 500U

typedef enum
{
    MOTOR_STOPPED = 0,
    MOTOR_CW,       /* 顺时针 */
    MOTOR_CCW       /* 逆时针 */
} Motor_State;

typedef struct
{
    volatile Motor_State state;
    volatile uint8_t     current_step;
    volatile uint16_t    step_us;
    volatile uint32_t    target_steps;  /* 0 = 连续旋转 */
    volatile uint32_t    step_count;
    TIM_HandleTypeDef   *htim;
} Stepper_Motor;

extern Stepper_Motor my_motor;

void StepperMotor_Init(TIM_HandleTypeDef *htim);
void StepperMotor_SetSpeed(uint16_t step_us);
void StepperMotor_Rotate(Motor_State direction, uint32_t steps, uint16_t step_us);
void StepperMotor_RotateAngle(Motor_State direction, float angle_degrees, uint16_t step_us);
void StepperMotor_Start(Motor_State direction, uint16_t step_us);
void StepperMotor_Stop(void);
void StepperMotor_HoldPosition(void);
uint8_t StepperMotor_IsRunning(void);
uint32_t StepperMotor_AngleToSteps(float angle_degrees);

/* TIM6 更新中断里调用 */
void StepperMotor_TIM6_Callback(TIM_HandleTypeDef *htim);

#endif /* __STEPPER_MOTOR_H */
