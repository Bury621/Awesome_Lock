#ifndef __UI_VIEWS_H
#define __UI_VIEWS_H

#include "main.h"
#include "lcd.h"
#include <stdint.h>

/*
 * 纯显示层：只负责把界面画到 LCD 帧缓冲并刷新，不包含任何业务判断。
 * 所有函数都在主循环上下文调用。
 */
typedef enum
{
    UI_INPUT_PWD = 0,   /* 开门密码 */
    UI_INPUT_ADMIN,     /* 管理员密码 */
    UI_INPUT_NEW        /* 设置新密码 */
} UI_InputMode;

void UI_ShowMain(void);
void UI_ShowInput(UI_InputMode mode, uint8_t dots);
void UI_ShowInputDots(uint8_t dots);
void UI_ShowSuccess(void);
void UI_ShowLocked(uint32_t remaining_seconds);
void UI_ShowMenu(uint8_t selected);
void UI_ShowEnrollPrompt(void);
void UI_ShowMessage(const char *line1, const char *line2, uint16_t color);

uint8_t UI_MenuItemCount(void);
const char *UI_MenuItemLabel(uint8_t index);

#endif /* __UI_VIEWS_H */
