#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "lcd_font.h"

// LCD尺寸定义 - 竖屏 160x128
#define LCD_WIDTH   128
#define LCD_HEIGHT  160

// 引脚定义
#define LCD_BL_PIN   GPIO_PIN_14
#define LCD_CS_PIN   GPIO_PIN_13
#define LCD_DC_PIN   GPIO_PIN_12
#define LCD_RES_PIN  GPIO_PIN_11

#define LCD_BL_PORT  GPIOB
#define LCD_CS_PORT  GPIOB
#define LCD_DC_PORT  GPIOB
#define LCD_RES_PORT GPIOB

// 函数声明
void LCD_Init(void);
void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteData16(uint16_t data);
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Clear(uint16_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

// 显示函数
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color);
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FontDef font, uint16_t color);
void LCD_ShowBedImage(uint16_t x, uint16_t y);
	
// 中文显示函数
void LCD_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t char_index, uint16_t color);
void LCD_ShowChineseString16x16(uint16_t x, uint16_t y, const uint8_t *char_indexes, uint8_t count, uint16_t color);

// 控制函数
void LCD_SetDirection(uint8_t direction);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_SetBacklight(uint8_t state);
#endif