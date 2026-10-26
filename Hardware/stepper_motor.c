#include "stepper_motor.h"

// 全局电机实例
Stepper_Motor my_motor;

// 步进序列（8步半步模式）
static const uint8_t step_sequence[STEP_SEQUENCE_SIZE] = {
    0b0001,  // IN1
    0b0011,  // IN1 + IN2
    0b0010,  // IN2
    0b0110,  // IN2 + IN3
    0b0100,  // IN3
    0b1100,  // IN3 + IN4
    0b1000,  // IN4
    0b1001   // IN4 + IN1
};

/**
 * @brief 设置线圈引脚状态
 * @param pattern: 线圈控制位图
 */
static void set_coils(uint8_t pattern)
{
    HAL_GPIO_WritePin(MOTOR_IN1_PORT, MOTOR_IN1_PIN, (pattern & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN2_PORT, MOTOR_IN2_PIN, (pattern & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN3_PORT, MOTOR_IN3_PIN, (pattern & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN4_PORT, MOTOR_IN4_PIN, (pattern & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 处理步进
 */
static void process_step(void)
{
    if(my_motor.state == MOTOR_STOPPED) {
        return;
    }
    
    // 执行一步
    if(my_motor.state == MOTOR_CW) {
        my_motor.current_step++;
        if(my_motor.current_step >= STEP_SEQUENCE_SIZE) {
            my_motor.current_step = 0;
        }
    } else {
        if(my_motor.current_step == 0) {
            my_motor.current_step = STEP_SEQUENCE_SIZE;
        }
        my_motor.current_step--;
    }
    
    // 设置线圈
    set_coils(step_sequence[my_motor.current_step]);
    
    // 步数计数
    my_motor.step_count++;
    
    // 检查是否达到目标步数
    if(my_motor.target_steps > 0 && my_motor.step_count >= my_motor.target_steps) {
        StepperMotor_Stop();
    }
}

/**
 * @brief 初始化步进电机
 * @param htim: 定时器句柄指针
 */
void StepperMotor_Init(TIM_HandleTypeDef *htim)
{
    my_motor.state = MOTOR_STOPPED;
    my_motor.current_step = 0;
    my_motor.speed_us = MOTOR_DEFAULT_SPEED_US;
    my_motor.target_steps = 0;
    my_motor.step_count = 0;
    my_motor.htim = htim;
    
    StepperMotor_SetSpeed(MOTOR_DEFAULT_SPEED_US);
    StepperMotor_Stop();  // 初始化时停止，默认不保持扭矩
}

/**
 * @brief 设置速度（单位：微秒）
 * @param speed_us: 步进间隔时间（微秒）
 */
void StepperMotor_SetSpeed(uint16_t speed_us)
{
    // 限制速度范围
    if(speed_us < MOTOR_MIN_SPEED_US) speed_us = MOTOR_MIN_SPEED_US;
    if(speed_us > MOTOR_MAX_SPEED_US) speed_us = MOTOR_MAX_SPEED_US;
    
    my_motor.speed_us = speed_us;
    
    // 设置定时器重载值（重载值 = 周期 - 1）
    uint32_t reload_value = speed_us - 1;
    
    if(my_motor.state != MOTOR_STOPPED) {
        __HAL_TIM_SET_AUTORELOAD(my_motor.htim, reload_value);
        __HAL_TIM_SET_COUNTER(my_motor.htim, 0);
    }
}

/**
 * @brief 修正的反转校准函数 - 与process_step反转逻辑完全一致
 */
void StepperMotor_Calibrate_CCW(void)
{
    const uint8_t Calibrate_step = 1;
    const uint8_t Calibrate_delay = 5;
    
    // 停止定时器
    HAL_TIM_Base_Stop_IT(my_motor.htim);
    
    // 设置反转状态
    my_motor.state = MOTOR_CCW;
    
    // 使用与process_step完全相同的反转逻辑
    for(int cycle = 0; cycle < Calibrate_step; cycle++) {
        for(int i = 0; i < STEP_SEQUENCE_SIZE; i++) {
            // 完全复制process_step中的反转逻辑
            if(my_motor.current_step == 0) {
                my_motor.current_step = STEP_SEQUENCE_SIZE;
            }
            my_motor.current_step--;
            
            set_coils(step_sequence[my_motor.current_step]);
            HAL_Delay(Calibrate_delay);
        }
    }
    
    my_motor.state = MOTOR_STOPPED;
    // 校准完成后调用标准的停止函数
    StepperMotor_Stop();
}

/**
 * @brief 优化的正转校准函数
 */
void StepperMotor_Calibrate(void)
{
    const uint8_t Calibrate_step = 1; // 校准次数
    const uint8_t Calibrate_delay = 5; // 校准延时
    
    // 停止定时器
    HAL_TIM_Base_Stop_IT(my_motor.htim);
    
    // 设置正转状态
    my_motor.state = MOTOR_CW;
    my_motor.current_step = 0;
    
    for(int cycle = 0; cycle < Calibrate_step; cycle++) {
        for(int step = 0; step < STEP_SEQUENCE_SIZE; step++) {
            // 使用与实际process_step中完全相同的正转逻辑
            my_motor.current_step++;
            if(my_motor.current_step >= STEP_SEQUENCE_SIZE) {
                my_motor.current_step = 0;
            }
            
            set_coils(step_sequence[my_motor.current_step]);
            HAL_Delay(Calibrate_delay);
        }
    }
    
    my_motor.state = MOTOR_STOPPED;
    // 校准完成后调用标准的停止函数
    StepperMotor_Stop();
}

/**
 * @brief 角度转换为步数
 * @param angle_degrees: 角度值（度）
 * @return 对应的步数
 */
uint32_t StepperMotor_AngleToSteps(float angle_degrees)
{
    // 28BYJ-48: 每转4096步，每步0.08789°
    return (uint32_t)(angle_degrees * (4096.0 / 360.0));
}

/**
 * @brief 旋转指定角度
 * @param direction: 旋转方向
 * @param angle_degrees: 角度值（度）
 * @param speed_us: 步进间隔时间（微秒）
 */
void StepperMotor_RotateAngle(Motor_State direction, float angle_degrees, uint16_t speed_us)
{
    uint32_t steps = StepperMotor_AngleToSteps(angle_degrees);
    StepperMotor_Rotate(direction, steps, speed_us);
}

/**
 * @brief 启动电机连续旋转
 * @param direction: 旋转方向
 * @param speed_us: 步进间隔时间（微秒）
 */
void StepperMotor_Start(Motor_State direction, uint16_t speed_us)
{
    my_motor.state = direction;
    my_motor.target_steps = 0;  // 0表示连续旋转
    my_motor.step_count = 0;
    
    // 设置速度
    StepperMotor_SetSpeed(speed_us);
    
    // 启动定时器
    __HAL_TIM_SET_COUNTER(my_motor.htim, 0);
    HAL_TIM_Base_Start_IT(my_motor.htim);
}

/**
 * @brief 旋转指定步数
 * @param direction: 旋转方向
 * @param steps: 步数
 * @param speed_us: 步进间隔时间（微秒）
 */
void StepperMotor_Rotate(Motor_State direction, uint32_t steps, uint16_t speed_us)
{
    my_motor.state = direction;
    my_motor.target_steps = steps;
    my_motor.step_count = 0;
    
    // 设置速度
    StepperMotor_SetSpeed(speed_us);
    
    // 启动定时器
    __HAL_TIM_SET_COUNTER(my_motor.htim, 0);
    HAL_TIM_Base_Start_IT(my_motor.htim);
}

/**
 * @brief 停止电机并断电（不保持扭矩）
 */
void StepperMotor_Stop(void)
{
    my_motor.state = MOTOR_STOPPED;
    my_motor.target_steps = 0;
    my_motor.step_count = 0;
    
    // 停止定时器
    if(my_motor.htim != NULL) {
        HAL_TIM_Base_Stop_IT(my_motor.htim);
    }
    
    // 断电，不保持扭矩
    set_coils(0x00);
}

/**
 * @brief 保持当前位置的扭矩
 * 手动调用以保持当前位置的扭矩
 */
void StepperMotor_HoldPosition(void)
{
    // 无论当前状态如何，都设置为停止状态
    my_motor.state = MOTOR_STOPPED;
    
    // 停止定时器
    if(my_motor.htim != NULL) {
        HAL_TIM_Base_Stop_IT(my_motor.htim);
    }
    
    // 保持当前步序的线圈状态，通常需要至少两个线圈通电以获得足够扭矩
    uint8_t hold_pattern = step_sequence[my_motor.current_step];
    
    // 如果是单线圈状态（0b0001, 0b0010, 0b0100, 0b1000），
    // 需要额外通电相邻线圈以获得更大保持扭矩
    switch(hold_pattern) {
        case 0b0001: // 只有IN1，需要加上IN4
            hold_pattern = 0b1001; // IN4 + IN1
            break;
        case 0b0010: // 只有IN2，需要加上IN1
            hold_pattern = 0b0011; // IN1 + IN2
            break;
        case 0b0100: // 只有IN3，需要加上IN2
            hold_pattern = 0b0110; // IN2 + IN3
            break;
        case 0b1000: // 只有IN4，需要加上IN3
            hold_pattern = 0b1100; // IN3 + IN4
            break;
        // 对于双线圈状态（0b0011, 0b0110, 0b1100, 0b1001），直接使用
    }
    
    set_coils(hold_pattern);
}

/**
 * @brief 检查电机是否在运行
 * @return 1:运行中, 0:停止
 */
uint8_t StepperMotor_IsRunning(void)
{
    return (my_motor.state != MOTOR_STOPPED);
}

/**
 * @brief TIM6中断回调函数
 * @param htim: 定时器句柄指针
 */
void StepperMotor_TIM6_Callback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6) {
        process_step();
    }
}
