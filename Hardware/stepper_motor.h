#ifndef __STEPPER_MOTOR_H
#define __STEPPER_MOTOR_H

#include "stm32f4xx_hal.h"

// 电机引脚定义 (PE11-PE14)
#define MOTOR_IN1_PIN GPIO_PIN_11
#define MOTOR_IN1_PORT GPIOE
#define MOTOR_IN2_PIN GPIO_PIN_12
#define MOTOR_IN2_PORT GPIOE
#define MOTOR_IN3_PIN GPIO_PIN_13
#define MOTOR_IN3_PORT GPIOE
#define MOTOR_IN4_PIN GPIO_PIN_14
#define MOTOR_IN4_PORT GPIOE

// 电机参数
#define STEPS_PER_REVOLUTION 4096
#define STEP_SEQUENCE_SIZE 8

// 速度范围定义（基于1MHz定时器）
#define MOTOR_MIN_SPEED_US 50
#define MOTOR_MAX_SPEED_US 500
#define MOTOR_DEFAULT_SPEED_US 200

// 电机状态枚举
typedef enum {
    MOTOR_STOPPED = 0,
    MOTOR_CW,     // 顺时针
    MOTOR_CCW     // 逆时针
} Motor_State;

// 电机控制结构体
typedef struct {
    Motor_State state;
    uint8_t current_step;
    uint16_t speed_us;
    uint32_t target_steps;
    uint32_t step_count;
    TIM_HandleTypeDef *htim;
} Stepper_Motor;

// 全局电机实例
extern Stepper_Motor my_motor;

// 函数声明
void StepperMotor_Init(TIM_HandleTypeDef *htim);
void StepperMotor_Start(Motor_State direction, uint16_t speed_us);
void StepperMotor_Stop(void);
void StepperMotor_Rotate(Motor_State direction, uint32_t steps, uint16_t speed_us);
void StepperMotor_SetSpeed(uint16_t speed_us);
void StepperMotor_Calibrate(void);
void StepperMotor_Calibrate_CCW(void);
uint8_t StepperMotor_IsRunning(void);
void StepperMotor_HoldPosition(void);

// 中断回调函数声明
void StepperMotor_TIM6_Callback(TIM_HandleTypeDef *htim);

// 角度转换函数
uint32_t StepperMotor_AngleToSteps(float angle_degrees);
void StepperMotor_RotateAngle(Motor_State direction, float angle_degrees, uint16_t speed_us);

#endif /* __STEPPER_MOTOR_H */
