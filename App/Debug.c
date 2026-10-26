#include "Debug.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
extern UART_HandleTypeDef huart1;
void Print_Boot_Info(void)
{
	uint32_t boot_time=HAL_GetTick();
    uart_printf("\r\n");
    uart_printf("========================================\r\n");
    uart_printf("    Awesome Lock System Boot Up\r\n");
    uart_printf("========================================\r\n");
    uart_printf("MCU: STM32F407VET6\r\n");
    uart_printf("Clock: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
    uart_printf("UART Baud: 9600\r\n");
    uart_printf("Flash: %lu KB\r\n", (*(uint16_t*)0x1FFF7A22));
    uart_printf("SRAM: 192 KB\r\n");
    uart_printf("Boot Time: %lu ms\r\n", boot_time);
    uart_printf("----------------------------------------\r\n");
    uart_printf("System Configuration:\r\n");
    uart_printf("EEPROM Addr: 0x0800C000\r\n");
    uart_printf("Flash Sector: 3\r\n");
    uart_printf("Password Max Length: %d\r\n", PWD_MAX_LEN);
    uart_printf("Password Min Length: %d\r\n", PWD_MIN_LEN);
    uart_printf("Open Door Angle: %ddeg\r\n", OPEN_DOOR_ANGLE);
    uart_printf("Open Door Speed: %d\r\n", OPEN_DOOR_SPEED);
    uart_printf("Door Open/Close Interval: %d seconds\r\n", OPEN_DOOR_TIME);
    uart_printf("Return to Main Time: %d seconds\r\n", OPEN_DOOR_RETURN_MAIN_TIME);
    uart_printf("Lock Fail Count: %d\r\n", FAIL_LOCK_COUNT);
    uart_printf("Lock Time: %d seconds\r\n", FAIL_LOCK_TIME);
    #ifdef MOTOR_REVERSE
    uart_printf("Motor Direction: Reverse\r\n");
    #else
    uart_printf("Motor Direction: Normal\r\n");
    #endif
    uart_printf("========================================\r\n");
    uart_printf("System Ready.\r\n");
    uart_printf("========================================\r\n\r\n");
}
void uart_printf(const char *format, ...)
{
    char buffer[128];  // 发送缓冲区
    va_list args;
    
    // 1. 使用vsprintf格式化字符串
    va_start(args, format);
    int len = vsprintf(buffer, format, args);  // 格式化到缓冲区
    va_end(args);
    
    // 2. 统计字符串长度
    // vsprintf已经返回了长度，但我们可以双重检查
    if (len <= 0) {
        return;  // 格式化失败
    }
    
    // 确保不超过缓冲区大小
    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }
    
    // 3. 填充USART发送参数并发送
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, len, 1000);
}
void print_binary16(uint16_t num)
{
    for(int i = 15; i >= 0; i--) {
        uart_printf("%d", (num >> i) & 0x01);
    }
    uart_printf("\r\n");
}
void uart_print_array(uint8_t array[],uint8_t len)
{
	for(uint8_t x=0;x<len;x++)
	{
		uart_printf("0X%02X ",array[x]);
	}
	uart_printf("\r\n");
}