#include "Keyboard.h"

/* 0x0852*741DCBA#963：与原工程按键板一致 */
static const char s_key_map[] = "0852*741DCBA#963";

static uint32_t s_last_recover_tick;
static uint8_t  s_error_count;

/* 粗略的微秒延时：168MHz 下约每 8 次循环 1us，用于软件 I2C 恢复 */
static void delay_short(void)
{
    for (volatile uint32_t i = 0U; i < 120U; i++)
    {
        __NOP();
    }
}

void Keyboard_RecoverBus(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* 1ms 内最多恢复一次，避免故障时反复 DeInit/Init 抢占 CPU */
    if ((HAL_GetTick() - s_last_recover_tick) < 1000U)
    {
        return;
    }
    s_last_recover_tick = HAL_GetTick();

    (void)HAL_I2C_DeInit(&hi2c1);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = KEYBOARD_SCL_Pin | KEYBOARD_SDA_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    /*
     * 从机卡住 SDA 时，主机发 9 个时钟把它“顶”出来，再补一个 STOP。
     */
    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SDA_Pin, GPIO_PIN_SET);
    for (uint8_t i = 0U; i < 9U; i++)
    {
        HAL_GPIO_WritePin(GPIOB, KEYBOARD_SCL_Pin, GPIO_PIN_RESET);
        delay_short();
        HAL_GPIO_WritePin(GPIOB, KEYBOARD_SCL_Pin, GPIO_PIN_SET);
        delay_short();
    }

    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SDA_Pin, GPIO_PIN_RESET);
    delay_short();
    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SCL_Pin, GPIO_PIN_SET);
    delay_short();
    HAL_GPIO_WritePin(GPIOB, KEYBOARD_SDA_Pin, GPIO_PIN_SET);
    delay_short();

    /* 重新初始化 I2C 与引脚复用 */
    (void)HAL_I2C_Init(&hi2c1);
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
        s_error_count = 0U;
        *key_data = (uint16_t)(((uint16_t)rx_data[0] << 8) | rx_data[1]);
        return true;
    }

    /* I2C 偶发错误属于正常现象（热插拔、干扰），累计后尝试恢复总线 */
    if (s_error_count < 0xFFU)
    {
        s_error_count++;
    }
    if (s_error_count >= 3U)
    {
        s_error_count = 0U;
        Keyboard_RecoverBus();
    }

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

void Keyboard_Init(void)
{
    uint16_t dummy;

    s_last_recover_tick = 0U;
    s_error_count = 0U;

    /* 上电探测一次：失败不阻塞、不报错，主循环会继续重试 */
    (void)Keyboard_ReadRaw(&dummy);

    /* 清掉上电期间可能挂起的按键中断标志 */
    __HAL_GPIO_EXTI_CLEAR_IT(KEYBOARD_IRQ_Pin);
}
