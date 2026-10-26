#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "lcd_font.h"

/* LCD 尺寸：竖屏 128x160 */
#define LCD_WIDTH   128U
#define LCD_HEIGHT  160U

/* 16x16 中文字模数量（与 lcd_font.c 中 Chinese_16x16 保持一致） */
#define LCD_CHINESE_16X16_COUNT 43U

/* 引脚定义 */
#define LCD_BL_PIN   GPIO_PIN_14
#define LCD_CS_PIN   GPIO_PIN_13
#define LCD_DC_PIN   GPIO_PIN_12
#define LCD_RES_PIN  GPIO_PIN_11

#define LCD_BL_PORT  GPIOB
#define LCD_CS_PORT  GPIOB
#define LCD_DC_PORT  GPIOB
#define LCD_RES_PORT GPIOB

/*
 * 重构说明：
 * 旧版每个像素都要单独发起一次 SPI 传输并翻转片选，整屏清屏上千万次 GPIO 操作，
 * 而且这些绘制全部发生在中断里，是“卡死”的主要放大器之一。
 *
 * 新版改为 RAM 帧缓冲（128*160*2 = 40KB），所有绘图函数只写内存，
 * 需要显示时调用 LCD_Flush()/LCD_FlushArea() 一次性批量送屏。
 * 优点：
 *   - 绘制速度快几个数量级，且不再占用中断时间；
 *   - 文字透明叠加、局部刷新都变得简单；
 *   - 不会出现画到一半被其他逻辑打断导致的闪烁。
 *
 * 注意：调用任何 LCD_xxx 绘图函数后，必须调用 LCD_Flush() 才会真正显示。
 */
void LCD_Init(void);
void LCD_Flush(void);
void LCD_FlushArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/* 底层（一般不需要直接调用） */
void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteData16(uint16_t data);
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Clear(uint16_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawHLine(uint16_t x1, uint16_t x2, uint16_t y, uint16_t color);
void LCD_DrawVLine(uint16_t x, uint16_t y1, uint16_t y2, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/* 显示函数 */
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color);
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FontDef font, uint16_t color);
void LCD_ShowBedImage(uint16_t x, uint16_t y);

/* 中文显示 */
void LCD_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t char_index, uint16_t color);
void LCD_ShowChineseString16x16(uint16_t x, uint16_t y, const uint8_t *char_indexes,
                                uint8_t count, uint16_t color);

/* 控制函数 */
void LCD_SetDirection(uint8_t direction);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_SetBacklight(uint8_t state);
uint8_t LCD_GetBacklight(void);
uint8_t LCD_IsFaulted(void);

#endif /* __LCD_H */
