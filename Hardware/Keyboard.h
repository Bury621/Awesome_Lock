#ifndef __KEYBOARD_H
#define __KEYBOARD_H
#include "main.h"

#define KEYBOARD_ADDRESS 0x65 //七位右对齐地址
extern I2C_HandleTypeDef hi2c1;


uint16_t Keyboard_get(void);
char Keyboard_GetChar(uint16_t key_data);
#endif