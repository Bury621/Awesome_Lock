#include "stepper_motor.h"
#include "app_util.h"

/* 全局电机实例 */
Stepper_Motor my_motor;

/* 8 拍半步序列 */
static const uint8_t s_step_sequence[STEP_SEQUENCE_SIZE] = {
    0x01U, /* IN1 */
    0x03U, /* IN1 + IN2 */
    0x02U, /* IN2 */
    0x06U, /* IN2 + IN3 */
    0x04U, /* IN3 */
    0x0CU, /* IN3 + IN4 */
    0x08U, /* IN4 */
    0x09U  /* IN4 + IN1 */
};

static void set_coils(uint8_t pattern)
{
    HAL_GPIO_WritePin(MOTOR_IN1_PORT, MOTOR_IN1_PIN,
                      ((pattern & 0x01U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN2_PORT, MOTOR_IN2_PIN,
                      ((pattern & 0x02U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN3_PORT, MOTOR_IN3_PIN,
                      ((pattern & 0x04U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN4_PORT, MOTOR_IN4_PIN,
                      ((pattern & 0x08U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 停止定时器并断电（可在中断里调用） */
static void motor_shutdown(void)
{
    my_motor.state = MOTOR_STOPPED;
    my_motor.target_steps = 0U;
    my_motor.step_count = 0U;

    if (my_motor.htim != NULL)
    {
        (void)HAL_TIM_Base_Stop_IT(my_motor.htim);
    }
    set_coils(0x00U);
}

static void process_step(void)
{
    if (my_motor.state == MOTOR_STOPPED)
    {
        return;
    }

    if (my_motor.state == MOTOR_CW)
    {
        my_motor.current_step++;
        if (my_motor.current_step >= STEP_SEQUENCE_SIZE)
        {
            my_motor.current_step = 0U;
        }
    }
    else
    {
        if (my_motor.current_step == 0U)
        {
            my_motor.current_step = STEP_SEQUENCE_SIZE;
        }
        my_motor.current_step--;
    }

    set_coils(s_step_sequence[my_motor.current_step]);
    my_motor.step_count++;

    if ((my_motor.target_steps > 0U) && (my_motor.step_count >= my_motor.target_steps))
    {
        motor_shutdown();
    }
}

void StepperMotor_Init(TIM_HandleTypeDef *htim)
{
    my_motor.htim = htim;
    my_motor.state = MOTOR_STOPPED;
    my_motor.current_step = 0U;
    my_motor.step_us = MOTOR_DEFAULT_STEP_US;
    my_motor.target_steps = 0U;
    my_motor.step_count = 0U;

    set_coils(0x00U);
    if (htim != NULL)
    {
        (void)HAL_TIM_Base_Stop_IT(htim);
        __HAL_TIM_SET_AUTORELOAD(htim, (MOTOR_DEFAULT_STEP_US - 1U));
        __HAL_TIM_SET_COUNTER(htim, 0U);
    }
}

void StepperMotor_SetSpeed(uint16_t step_us)
{
    if (step_us < MOTOR_MIN_STEP_US)
    {
        step_us = MOTOR_MIN_STEP_US;
    }
    if (step_us > MOTOR_MAX_STEP_US)
    {
        step_us = MOTOR_MAX_STEP_US;
    }

    my_motor.step_us = step_us;

    if (my_motor.htim != NULL)
    {
        /* 始终更新，避免启动第一步仍使用旧速度 */
        __HAL_TIM_SET_AUTORELOAD(my_motor.htim, (uint32_t)(step_us - 1U));
    }
}

void StepperMotor_Rotate(Motor_State direction, uint32_t steps, uint16_t step_us)
{
    if ((direction == MOTOR_STOPPED) || (steps == 0U) || (my_motor.htim == NULL))
    {
        return;
    }

    uint32_t primask = App_EnterCritical();

    my_motor.state = direction;
    my_motor.target_steps = steps;
    my_motor.step_count = 0U;

    StepperMotor_SetSpeed(step_us);
    __HAL_TIM_SET_COUNTER(my_motor.htim, 0U);
    (void)HAL_TIM_Base_Start_IT(my_motor.htim);

    App_ExitCritical(primask);
}

void StepperMotor_RotateAngle(Motor_State direction, float angle_degrees, uint16_t step_us)
{
    StepperMotor_Rotate(direction, StepperMotor_AngleToSteps(angle_degrees), step_us);
}

void StepperMotor_Start(Motor_State direction, uint16_t step_us)
{
    if ((direction == MOTOR_STOPPED) || (my_motor.htim == NULL))
    {
        return;
    }

    uint32_t primask = App_EnterCritical();

    my_motor.state = direction;
    my_motor.target_steps = 0U; /* 连续旋转 */
    my_motor.step_count = 0U;

    StepperMotor_SetSpeed(step_us);
    __HAL_TIM_SET_COUNTER(my_motor.htim, 0U);
    (void)HAL_TIM_Base_Start_IT(my_motor.htim);

    App_ExitCritical(primask);
}

void StepperMotor_Stop(void)
{
    uint32_t primask = App_EnterCritical();
    motor_shutdown();
    App_ExitCritical(primask);
}

void StepperMotor_HoldPosition(void)
{
    uint32_t primask = App_EnterCritical();
    uint8_t hold_pattern;

    my_motor.state = MOTOR_STOPPED;
    my_motor.target_steps = 0U;
    my_motor.step_count = 0U;

    if (my_motor.htim != NULL)
    {
        (void)HAL_TIM_Base_Stop_IT(my_motor.htim);
    }

    hold_pattern = s_step_sequence[my_motor.current_step];

    /* 单线圈状态补上相邻线圈，获得更大的保持扭矩 */
    switch (hold_pattern)
    {
        case 0x01U: hold_pattern = 0x09U; break;
        case 0x02U: hold_pattern = 0x03U; break;
        case 0x04U: hold_pattern = 0x06U; break;
        case 0x08U: hold_pattern = 0x0CU; break;
        default: break;
    }

    set_coils(hold_pattern);
    App_ExitCritical(primask);
}

uint8_t StepperMotor_IsRunning(void)
{
    uint8_t running;
    uint32_t primask = App_EnterCritical();
    running = (my_motor.state != MOTOR_STOPPED) ? 1U : 0U;
    App_ExitCritical(primask);
    return running;
}

uint32_t StepperMotor_AngleToSteps(float angle_degrees)
{
    if (angle_degrees <= 0.0f)
    {
        return 0U;
    }
    return (uint32_t)((angle_degrees * (float)STEPS_PER_REVOLUTION / 360.0f) + 0.5f);
}

void StepperMotor_TIM6_Callback(TIM_HandleTypeDef *htim)
{
    if ((htim != NULL) && (htim->Instance == TIM6))
    {
        process_step();
    }
}
