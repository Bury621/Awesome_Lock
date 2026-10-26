#include "Debug.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart1;

#ifndef DEBUG_UART_ENABLE
#define DEBUG_UART_ENABLE 0
#endif

void uart_printf(const char *format, ...)
{
#if DEBUG_UART_ENABLE
    /*
     * 静态缓冲区：uart_printf 只在主循环/启动阶段调用，不会并发，
     * 这样可以从本就很小的主栈里省下 160 字节。
     */
    static char buffer[160];
    va_list args;
    int len;

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }
    if (len >= (int)sizeof(buffer))
    {
        len = (int)sizeof(buffer) - 1;
    }

    (void)HAL_UART_Transmit(&huart1, (uint8_t *)buffer, (uint16_t)len, 1000U);
#else
    /*
     * 默认关闭调试串口输出：
     * 现场没有接 USART1 时，HAL_UART_Transmit 会占用主循环甚至等待 tick；
     * 电机/指纹路径里任何一条日志都可能把卡死放大。
     * 需要抓日志时把这里改成 1 重新编译。
     */
    (void)format;
#endif
}

void print_binary16(uint16_t num)
{
    for (int i = 15; i >= 0; i--)
    {
        uart_printf("%d", (num >> i) & 0x01);
    }
    uart_printf("\r\n");
}

void uart_print_array(const uint8_t array[], uint8_t len)
{
    if (array == NULL)
    {
        return;
    }

    for (uint8_t x = 0U; x < len; x++)
    {
        uart_printf("0X%02X ", array[x]);
    }
    uart_printf("\r\n");
}

void Print_Boot_Info(void)
{
    uart_printf("\r\n");
    uart_printf("========================================\r\n");
    uart_printf("    Awesome Lock System Boot Up\r\n");
    uart_printf("========================================\r\n");
    uart_printf("MCU: STM32F407VET6\r\n");
    uart_printf("Clock: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000U);
    uart_printf("UART Baud: 9600\r\n");
    uart_printf("Flash: %lu KB\r\n", (unsigned long)(*(uint16_t *)0x1FFF7A22U));
    uart_printf("SRAM: 192 KB\r\n");
    uart_printf("Boot Time: %lu ms\r\n", HAL_GetTick());
    uart_printf("----------------------------------------\r\n");
    uart_printf("EEPROM Addr: 0x%08lX\r\n", (unsigned long)FLASH_EEPROM_BASE_ADDR);
    uart_printf("Password Max Length: %d\r\n", PWD_MAX_LEN);
    uart_printf("Password Min Length: %d\r\n", PWD_MIN_LEN);
    uart_printf("Open Door Angle: %u deg\r\n", (unsigned)OPEN_DOOR_ANGLE);
    uart_printf("Open Door Step: %u us\r\n", (unsigned)OPEN_DOOR_STEP_US);
    uart_printf("Door Open/Close Interval: %u ms\r\n", (unsigned)OPEN_DOOR_TIME_MS);
    uart_printf("Return to Main Time: %u ms\r\n", (unsigned)OPEN_DOOR_HOLD_MS);
    uart_printf("Lock Fail Count: %u\r\n", (unsigned)FAIL_LOCK_COUNT);
    uart_printf("Lock Time: %u s\r\n", (unsigned)FAIL_LOCK_TIME);
#ifdef MOTOR_REVERSE
    uart_printf("Motor Direction: Reverse\r\n");
#else
    uart_printf("Motor Direction: Normal\r\n");
#endif
    uart_printf("========================================\r\n");
    uart_printf("System Ready.\r\n");
    uart_printf("========================================\r\n\r\n");
}
