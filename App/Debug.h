#ifndef __DEBUG_H
#define __DEBUG_H
#include "main.h"
void uart_printf(const char *format, ...);
void print_binary16(uint16_t num);
void Print_Boot_Info(void);
void uart_print_array(const uint8_t array[],uint8_t len);
#endif