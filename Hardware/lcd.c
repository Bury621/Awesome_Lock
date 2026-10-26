#include "lcd.h"
#include <string.h>

extern SPI_HandleTypeDef hspi2;

/*
 * 2026-10 回滚说明：
 * 之前把 LCD 改成了 40KB 帧缓冲 + 批量 SPI 刷新，实机出现中文显示异常/重启。
 * 为了优先保证可用性，这里恢复成与旧版完全一致的“逐点直接写屏”实现，
 * 只修复旧版的越界读和参数检查问题。
 *
 * UI 层仍然会调用 LCD_Flush()/LCD_FlushArea()，这两个函数现在是空操作，
 * 因为绘制已经直接写到了屏幕上。
 */
void LCD_Flush(void)
{
}

void LCD_FlushArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

/* 单次 SPI 传输超时，防止硬件异常时永久卡死 */
#define LCD_SPI_TIMEOUT_MS 100U

static uint8_t s_backlight_on;
static uint8_t s_lcd_fault;

/* 批量填充缓冲：最多一整列 160 像素，或一整行 128 像素 */
static uint8_t s_fill_buffer[LCD_HEIGHT * 2U];

/*
 * 所有 SPI 发送统一走这里。
 * 如果屏幕排线/SPI 异常导致 HAL 返回错误，只标记故障并停发，
 * 让主循环继续跑（按键、看门狗仍然工作），而不是每字节等 100ms 把启动拖死。
 */
static uint8_t lcd_spi_send(const uint8_t *data, uint16_t length)
{
    if (s_lcd_fault != 0U)
    {
        return 0U;
    }

    if (HAL_SPI_Transmit(&hspi2, (uint8_t *)data, length, LCD_SPI_TIMEOUT_MS) != HAL_OK)
    {
        s_lcd_fault = 1U;
        return 0U;
    }

    return 1U;
}

/* 面板是 BGR 顺序，发送前交换 R/B 分量 */
static uint16_t lcd_swap_rb(uint16_t color)
{
    return (uint16_t)(((color & 0xF800U) >> 11) |
                      (color & 0x07E0U) |
                      ((color & 0x001FU) << 11));
}

/* 把 count 个相同颜色打包成 SPI 字节流（高字节在前） */
static void lcd_pack_uniform_color(uint16_t color, uint16_t count)
{
    uint16_t swapped = lcd_swap_rb(color);
    uint8_t  high = (uint8_t)(swapped >> 8);
    uint8_t  low = (uint8_t)(swapped & 0xFFU);

    for (uint16_t i = 0U; i < count; i++)
    {
        s_fill_buffer[i * 2U] = high;
        s_fill_buffer[i * 2U + 1U] = low;
    }
}

/* CS 拉低后一次性发送一批像素数据，而不是每个字节调用一次 HAL */
static void lcd_send_pixels(const uint8_t *data, uint16_t length)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    (void)lcd_spi_send(data, length);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

// 写命令
void LCD_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
    
    (void)lcd_spi_send(&cmd, 1U);
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

// 写数据
void LCD_WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    (void)lcd_spi_send(&data, 1U);
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

void LCD_WriteData16(uint16_t data)
{
    uint16_t swapped = lcd_swap_rb(data);

    LCD_WriteData((uint8_t)(swapped >> 8));
    LCD_WriteData((uint8_t)(swapped & 0xFFU));
}

// 设置显示窗口
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // 添加偏移量
    x1 += 2;  // 右移2像素
    x2 += 2;  // 右移2像素
    y1 += 1;  // 下移1像素
    y2 += 1;  // 下移1像素
    
    // 限制坐标范围
    if(x1 >= LCD_WIDTH + 2) x1 = LCD_WIDTH + 2 - 1;
    if(x2 >= LCD_WIDTH + 2) x2 = LCD_WIDTH + 2 - 1;
    if(y1 >= LCD_HEIGHT + 1) y1 = LCD_HEIGHT + 1 - 1;
    if(y2 >= LCD_HEIGHT + 1) y2 = LCD_HEIGHT + 1 - 1;
    
    LCD_WriteCommand(0x2A); // 列地址设置
    LCD_WriteData(0x00);
    LCD_WriteData(x1);      // 起始列
    LCD_WriteData(0x00);
    LCD_WriteData(x2);      // 结束列
    
    LCD_WriteCommand(0x2B); // 行地址设置
    LCD_WriteData(0x00);
    LCD_WriteData(y1);      // 起始行
    LCD_WriteData(0x00);
    LCD_WriteData(y2);      // 结束行
    
    LCD_WriteCommand(0x2C); // 存储器写
}
// LCD初始化
void LCD_Init(void)
{
    // 初始化GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 背光引脚
    GPIO_InitStruct.Pin = LCD_BL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_BL_PORT, &GPIO_InitStruct);
    
    // 控制引脚
    GPIO_InitStruct.Pin = LCD_CS_PIN | LCD_DC_PIN | LCD_RES_PIN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 初始状态
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET);
	
	
    
    // 复位序列
    HAL_GPIO_WritePin(LCD_RES_PORT, LCD_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RES_PORT, LCD_RES_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RES_PORT, LCD_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(120);
    
    // 初始化命令序列
    LCD_WriteCommand(0x11); // Sleep out
    HAL_Delay(120);
    
    // 设置帧率
    LCD_WriteCommand(0xB1); // FRMCTR1
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    LCD_WriteCommand(0xB2); // FRMCTR2
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    LCD_WriteCommand(0xB3); // FRMCTR3
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    LCD_WriteCommand(0xB4); // INVCTR
    LCD_WriteData(0x07);
    
    // 电源控制
    LCD_WriteCommand(0xC0); // PWCTR1
    LCD_WriteData(0xA2);
    LCD_WriteData(0x02);
    LCD_WriteData(0x84);
    
    LCD_WriteCommand(0xC1); // PWCTR2
    LCD_WriteData(0xC5);
    
    LCD_WriteCommand(0xC2); // PWCTR3
    LCD_WriteData(0x0A);
    LCD_WriteData(0x00);
    
    LCD_WriteCommand(0xC3); // PWCTR4
    LCD_WriteData(0x8A);
    LCD_WriteData(0x2A);
    
    LCD_WriteCommand(0xC4); // PWCTR5
    LCD_WriteData(0x8A);
    LCD_WriteData(0xEE);
    
    LCD_WriteCommand(0xC5); // VMCTR1
    LCD_WriteData(0x0E);
    
    // 关闭显示反转
    LCD_WriteCommand(0x20); // Display Inversion Off
    
    // 设置内存访问控制
    LCD_WriteCommand(0x36); // Memory Data Access Control
    LCD_WriteData(0xC8);    // MY=1, MX=0, MV=1, ML=0, RGB=0, MH=0
    
    // 设置像素格式
    LCD_WriteCommand(0x3A); // COLMOD: Pixel Format Set
    LCD_WriteData(0x05);    // 16 bits per pixel
    
    // 设置显示区域
    LCD_WriteCommand(0x2A); // 列地址设置
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);    // 起始列 = 0
    LCD_WriteData(0x00);
    LCD_WriteData(0x7F);    // 结束列 = 127
    
    LCD_WriteCommand(0x2B); // 行地址设置
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);    // 起始行 = 0
    LCD_WriteData(0x00);
    LCD_WriteData(0x9F);    // 结束行 = 159
    
    // Gamma校正
    LCD_WriteCommand(0xE0); // Gamma Adjustment (+Polarity)
    LCD_WriteData(0x02);
    LCD_WriteData(0x1C);
    LCD_WriteData(0x07);
    LCD_WriteData(0x12);
    LCD_WriteData(0x37);
    LCD_WriteData(0x32);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x29);
    LCD_WriteData(0x25);
    LCD_WriteData(0x2B);
    LCD_WriteData(0x39);
    LCD_WriteData(0x00);
    LCD_WriteData(0x01);
    LCD_WriteData(0x03);
    LCD_WriteData(0x10);
    
    LCD_WriteCommand(0xE1); // Gamma Adjustment (-Polarity)
    LCD_WriteData(0x03);
    LCD_WriteData(0x1D);
    LCD_WriteData(0x07);
    LCD_WriteData(0x06);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x37);
    LCD_WriteData(0x3F);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x02);
    LCD_WriteData(0x10);
    
    // 打开显示
    LCD_WriteCommand(0x29); // Display On
    HAL_Delay(100);
    
    // 开启背光
    LCD_SetBacklight(1);
    
    // 清屏测试
    LCD_Clear(BLACK);
}

// 清屏
void LCD_Clear(uint16_t color)
{
    LCD_Fill(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U, color);
}

// 画点（文字/图片仍然逐点绘制，保证透明叠加效果不变）
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    LCD_SetWindow(x, y, x, y);
    LCD_WriteData16(color);
}

// 填充矩形：同一颜色的批量传输，整屏清屏从 40960 次 SPI 调用降到 160 次
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t width;
    uint32_t height;

    if(x1 >= LCD_WIDTH || y1 >= LCD_HEIGHT) return;
    if(x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1U;
    if(y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1U;
    if(x1 > x2) { uint16_t t = x1; x1 = x2; x2 = t; }
    if(y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }

    width = (uint32_t)(x2 - x1 + 1U);
    height = (uint32_t)(y2 - y1 + 1U);
    if((width == 0U) || (height == 0U)) return;

    lcd_pack_uniform_color(color, (uint16_t)width);

    LCD_SetWindow(x1, y1, x2, y2);
    for(uint32_t row = 0U; row < height; row++)
    {
        lcd_send_pixels(s_fill_buffer, (uint16_t)(width * 2U));
    }
}

// 画水平线
void LCD_DrawHLine(uint16_t x1, uint16_t x2, uint16_t y, uint16_t color)
{
    LCD_Fill(x1, y, x2, y, color);
}

// 画垂直线：整列一次发送，避免逐像素调用 HAL
void LCD_DrawVLine(uint16_t x, uint16_t y1, uint16_t y2, uint16_t color)
{
    uint32_t height;

    if(x >= LCD_WIDTH) return;
    if(y1 >= LCD_HEIGHT) y1 = LCD_HEIGHT - 1U;
    if(y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1U;
    if(y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }

    height = (uint32_t)(y2 - y1 + 1U);
    if(height == 0U) return;

    lcd_pack_uniform_color(color, (uint16_t)height);

    LCD_SetWindow(x, y1, x, y2);
    lcd_send_pixels(s_fill_buffer, (uint16_t)(height * 2U));
}

// 画矩形框
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawHLine(x1, x2, y1, color); // 上边
    LCD_DrawHLine(x1, x2, y2, color); // 下边
    LCD_DrawVLine(x1, y1, y2, color); // 左边
    LCD_DrawVLine(x2, y1, y2, color); // 右边
}

// 显示字符函数（无背景色，透明显示）
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color)
{
    uint32_t i, j;
    uint8_t temp;
    
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if((font.table == NULL) || (font.width == 0U) || (font.height == 0U)) return;

    /* 旧版对非 ASCII 字符会直接数组越界，这里统一替换成 '?' */
    if((ch < ' ') || (ch > '~')) ch = '?';
    
    for(i = 0; i < font.height; i++)
    {
        temp = font.table[((uint32_t)ch - (uint32_t)' ') * font.height + i];
        for(j = 0; j < font.width; j++)
        {
            // 改为从最低位开始扫描（水平翻转）
            if(temp & (0x01 << j))
            {
                LCD_DrawPoint(x + j, y + i, color);
            }
        }
    }
}

// 显示字符串（无背景色）
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color)
{
    if(str == NULL) return;

    while(*str)
    {
        LCD_ShowChar(x, y, *str, font, color);
        x += font.width;
        str++;
        
        // 如果超出屏幕宽度，换行
        if(x + font.width >= LCD_WIDTH)
        {
            x = 0;
            y += font.height;
        }
    }
}
// 数字转字符串（不依赖sprintf）
static void NumberToString(uint32_t num, char *str)
{
    uint8_t i = 0;
    uint8_t j = 0;
    char temp[20];
    
    // 处理0的情况
    if(num == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // 逐位转换
    while(num > 0)
    {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // 反转字符串
    while(i > 0)
    {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

// 显示数字
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FontDef font, uint16_t color)
{
    uint8_t t;
    char str[20];
    uint8_t num_len = 0;
    uint32_t temp_num = num;
    const char *digits = str;
    
    // 计算数字长度
    do {
        num_len++;
        temp_num /= 10;
    } while(temp_num > 0);
    
    // 数字转字符串
    NumberToString(num, str);

    /* 位数超过显示宽度时只显示低位，避免 len - num_len 下溢 */
    if(num_len > len)
    {
        digits = &str[num_len - len];
        num_len = len;
    }
    
    // 显示数字，前面补空格
    for(t = 0; t < len; t++)
    {
        if(t < len - num_len)
        {
            LCD_ShowChar(x + t * font.width, y, ' ', font, color);
        }
        else
        {
            LCD_ShowChar(x + t * font.width, y, digits[t - (len - num_len)], font, color);
        }
    }
}
void LCD_ShowBedImage(uint16_t x, uint16_t y)
{
    /*
     * gImage_bed 是 Image2Lcd 输出：8 字节文件头 + 48*48*2 像素数据 = 4616 字节。
     * 旧代码写成 +12，会越界读取 4 字节，并且整体像素错位。
     */
    const uint8_t *image_data = gImage_bed + 8;
    uint16_t width = 48;
    uint16_t height = 48;
    
    for(uint16_t i = 0; i < height; i++)
    {
        for(uint16_t j = 0; j < width; j++)
        {
            uint32_t index = (i * width + j) * 2;
            uint16_t pixel_color = (image_data[index] << 8) | image_data[index + 1];
            
            // 透明处理：跳过纯黑色
            if(pixel_color != 0x0000) {
                LCD_DrawPoint(x + j, y + i, pixel_color);
            }
        }
    }
	LCD_DrawPoint(x + 47, y + 47, SILVER);
	LCD_DrawPoint(x + 46, y + 47, SILVER);
}
void LCD_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t char_index, uint16_t color)
{
    uint32_t i, j;
    
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if(char_index >= LCD_CHINESE_16X16_COUNT) return;
    
    for(i = 0; i < 16; i++)
    {
        for(j = 0; j < 16; j++)
        {
            // 直接使用原字模数据，不进行翻转
            uint8_t byte_index = i * 2 + (j / 8);
            uint8_t bit_index = 7 - (j % 8);  // 位顺序不变
            
            if(Chinese_16x16[char_index][byte_index] & (0x80 >> bit_index))
            {
                LCD_DrawPoint(x + j, y + i, color);
            }
        }
    }
}

// 设置显示方向
void LCD_SetDirection(uint8_t direction)
{
    LCD_WriteCommand(0x36); // MADCTL
    LCD_WriteData(direction);
}
void LCD_ShowChineseString16x16(uint16_t x, uint16_t y, const uint8_t *char_indexes, uint8_t count, uint16_t color)
{
    uint8_t i;
    
    for(i = 0; i < count; i++)
    {
        LCD_ShowChinese16x16(x + i * 16, y, char_indexes[i], color);
    }
}
// 控制背光
void LCD_SetBacklight(uint8_t state)
{
    s_backlight_on = (state != 0U) ? 1U : 0U;
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, (s_backlight_on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t LCD_GetBacklight(void)
{
    return s_backlight_on;
}

uint8_t LCD_IsFaulted(void)
{
    return s_lcd_fault;
}

// 打开显示
void LCD_DisplayOn(void)
{
    LCD_WriteCommand(0x29);
}

// 关闭显示
void LCD_DisplayOff(void)
{
    LCD_WriteCommand(0x28);
}
