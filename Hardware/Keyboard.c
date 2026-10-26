#include "Keyboard.h"

/* 0x0852*741DCBA#963：与原工程按键板一致 */
static const char s_key_map[] = "0852*741DCBA#963";

/*
 * 2026-10 修正：
 * 旧重构版把超时缩短到 5ms，并在累计 3 次错误后反复 DeInit/Init I2C，
 * 实机上可能导致按键完全无响应。这里恢复旧版较宽松的时序，并取消自动 DeInit。
 */
#define KEYBOARD_I2C_TIMEOUT_MS 50U

void Keyboard_Init(void)
{
    /* 清掉上电期间可能挂起的按键中断标志，不主动读 I2C */
    __HAL_GPIO_EXTI_CLEAR_IT(KEYBOARD_IRQ_Pin);
}

bool Keyboard_ReadRaw(uint16_t *key_data)
{
    uint8_t rx_data[2] = {0U, 0U};
    HAL_StatusTypeDef status;

    if (key_data == NULL)
    {
        return false;
    }

    status = HAL_I2C_Master_Receive(&hi2c1,
                                    (uint16_t)((KEYBOARD_ADDRESS << 1) | 0x01U),
                                    rx_data, 2U, KEYBOARD_I2C_TIMEOUT_MS);
    if (status == HAL_OK)
    {
        *key_data = (uint16_t)(((uint16_t)rx_data[0] << 8) | rx_data[1]);
        return true;
    }

    /*
     * 读失败只返回 false，由主循环稍后重试。
     * 不再自动 HAL_I2C_DeInit/Init：反复初始化可能让按键模块彻底失联。
     */
    return false;
}

char Keyboard_GetChar(uint16_t key_data)
{
    uint8_t pos = 0U;
    uint16_t temp = key_data;

    /* 只接受“单键按下”，0 或多键同按都返回无效 */
    if ((key_data == 0U) || ((key_data & (uint16_t)(key_data - 1U)) != 0U))
    {
        return '\0';
    }

    while (temp > 1U)
    {
        temp >>= 1;
        pos++;
    }

    if (pos > 15U)
    {
        return '\0';
    }

    return s_key_map[15U - pos];
}

/*
 * 软件 I2C 总线恢复，保留为手动/调试用途。
 * 当前主流程不自动调用，避免干扰按键模块。
 */
void Keyboard_RecoverBus(void)
{
    GPIO_InitTypeDef gpio = {0};

    (void)HAL_I2C_DeInit(&hi2c1);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = KEYBOARD_SCL_Pin | KEYBOARD_SDA_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SDA_Pin, GPIO_PIN_SET);
    for (uint8_t i = 0U; i < 9U; i++)
    {
        HAL_GPIO_WritePin(GPIOB, KEYBOARD_SCL_Pin, GPIO_PIN_RESET);
        for (volatile uint32_t d = 0U; d < 120U; d++) { __NOP(); }
        HAL_GPIO_WritePin(GPIOB, KEYBOARD_SCL_Pin, GPIO_PIN_SET);
        for (volatile uint32_t d = 0U; d < 120U; d++) { __NOP(); }
    }

    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SDA_Pin, GPIO_PIN_RESET);
    for (volatile uint32_t d = 0U; d < 120U; d++) { __NOP(); }
    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SCL_Pin, GPIO_PIN_SET);
    for (volatile uint32_t d = 0U; d < 120U; d++) { __NOP(); }
    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SDA_Pin, GPIO_PIN_SET);

    (void)HAL_I2C_Init(&hi2c1);
}
