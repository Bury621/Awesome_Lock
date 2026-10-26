#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "main.h"
#include <stdbool.h>

/* 七位右对齐地址 */
#define KEYBOARD_ADDRESS        0x65U
#define KEYBOARD_I2C_TIMEOUT_MS 50U

extern I2C_HandleTypeDef hi2c1;

/*
 * 读取键盘原始键值（非阻塞，最多阻塞 5ms）。
 * 成功返回 true；I2C 异常返回 false，并在连续失败时自动做总线恢复。
 * 绝不调用 Error_Handler()，避免一个按键就把整机拖死。
 */
bool Keyboard_ReadRaw(uint16_t *key_data);

/* 把原始键值解析成字符，无效/多键同按返回 '\0' */
char Keyboard_GetChar(uint16_t key_data);

/* 上电自检与总线恢复（可在初始化时调用一次） */
void Keyboard_Init(void);
void Keyboard_RecoverBus(void);

#endif /* __KEYBOARD_H */
