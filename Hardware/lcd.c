#include "lcd.h"

extern SPI_HandleTypeDef hspi2;

/* -------------------------------------------------------------------------- */
/* 帧缓冲                                                                      */
/* -------------------------------------------------------------------------- */
static uint16_t s_framebuffer[LCD_WIDTH * LCD_HEIGHT];
static uint8_t  s_line_buffer[LCD_WIDTH * 2U];
static uint8_t  s_backlight_on;

/* 批量传输时给 HAL 的超时，防止 SPI 异常时永久阻塞 */
#define LCD_SPI_TIMEOUT_MS 100U

/* -------------------------------------------------------------------------- */
/* 底层 SPI                                                                    */
/* -------------------------------------------------------------------------- */
void LCD_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
    (void)HAL_SPI_Transmit(&hspi2, &cmd, 1U, LCD_SPI_TIMEOUT_MS);
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

void LCD_WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    (void)HAL_SPI_Transmit(&hspi2, &data, 1U, LCD_SPI_TIMEOUT_MS);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/* 面板是 BGR 顺序，发送前交换 R/B 分量 */
static inline uint16_t lcd_swap_rb(uint16_t color)
{
    return (uint16_t)(((color & 0xF800U) >> 11) |
                      (color & 0x07E0U) |
                      ((color & 0x001FU) << 11));
}

void LCD_WriteData16(uint16_t data)
{
    uint16_t swapped = lcd_swap_rb(data);
    LCD_WriteData((uint8_t)(swapped >> 8));
    LCD_WriteData((uint8_t)(swapped & 0xFFU));
}

void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* 面板可见区域相对显存有偏移 */
    x1 += 2U;
    x2 += 2U;
    y1 += 1U;
    y2 += 1U;

    if (x1 >= LCD_WIDTH + 2U)  { x1 = LCD_WIDTH + 1U; }
    if (x2 >= LCD_WIDTH + 2U)  { x2 = LCD_WIDTH + 1U; }
    if (y1 >= LCD_HEIGHT + 1U) { y1 = LCD_HEIGHT; }
    if (y2 >= LCD_HEIGHT + 1U) { y2 = LCD_HEIGHT; }

    LCD_WriteCommand(0x2A); /* 列地址 */
    LCD_WriteData(0x00);
    LCD_WriteData((uint8_t)x1);
    LCD_WriteData(0x00);
    LCD_WriteData((uint8_t)x2);

    LCD_WriteCommand(0x2B); /* 行地址 */
    LCD_WriteData(0x00);
    LCD_WriteData((uint8_t)y1);
    LCD_WriteData(0x00);
    LCD_WriteData((uint8_t)y2);

    LCD_WriteCommand(0x2C); /* 写显存 */
}

/* -------------------------------------------------------------------------- */
/* 批量刷新                                                                    */
/* -------------------------------------------------------------------------- */
void LCD_FlushArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint16_t x;
    uint16_t y;
    uint16_t width;

    if (x1 >= LCD_WIDTH)  { x1 = LCD_WIDTH - 1U; }
    if (y1 >= LCD_HEIGHT) { y1 = LCD_HEIGHT - 1U; }
    if (x2 >= LCD_WIDTH)  { x2 = LCD_WIDTH - 1U; }
    if (y2 >= LCD_HEIGHT) { y2 = LCD_HEIGHT - 1U; }
    if (x1 > x2) { uint16_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }

    width = (uint16_t)(x2 - x1 + 1U);

    LCD_SetWindow(x1, y1, x2, y2);

    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    for (y = y1; y <= y2; y++)
    {
        uint8_t *p = s_line_buffer;
        const uint16_t *src = &s_framebuffer[y * LCD_WIDTH + x1];

        for (x = 0U; x < width; x++)
        {
            uint16_t swapped = lcd_swap_rb(src[x]);
            *p++ = (uint8_t)(swapped >> 8);
            *p++ = (uint8_t)(swapped & 0xFFU);
        }

        (void)HAL_SPI_Transmit(&hspi2, s_line_buffer, (uint16_t)(width * 2U),
                               LCD_SPI_TIMEOUT_MS);
    }
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

void LCD_Flush(void)
{
    LCD_FlushArea(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U);
}

/* -------------------------------------------------------------------------- */
/* 初始化                                                                      */
/* -------------------------------------------------------------------------- */
void LCD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = LCD_BL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_BL_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_CS_PIN | LCD_DC_PIN | LCD_RES_PIN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET);
    s_backlight_on = 0U;

    /* 复位序列 */
    HAL_GPIO_WritePin(LCD_RES_PORT, LCD_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RES_PORT, LCD_RES_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RES_PORT, LCD_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(120);

    LCD_WriteCommand(0x11); /* Sleep out */
    HAL_Delay(120);

    LCD_WriteCommand(0xB1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    LCD_WriteCommand(0xB2);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    LCD_WriteCommand(0xB3);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    LCD_WriteCommand(0xB4);
    LCD_WriteData(0x07);

    LCD_WriteCommand(0xC0);
    LCD_WriteData(0xA2);
    LCD_WriteData(0x02);
    LCD_WriteData(0x84);

    LCD_WriteCommand(0xC1);
    LCD_WriteData(0xC5);

    LCD_WriteCommand(0xC2);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x00);

    LCD_WriteCommand(0xC3);
    LCD_WriteData(0x8A);
    LCD_WriteData(0x2A);

    LCD_WriteCommand(0xC4);
    LCD_WriteData(0x8A);
    LCD_WriteData(0xEE);

    LCD_WriteCommand(0xC5);
    LCD_WriteData(0x0E);

    LCD_WriteCommand(0x20); /* Display Inversion Off */

    LCD_WriteCommand(0x36); /* MADCTL */
    LCD_WriteData(0xC8);

    LCD_WriteCommand(0x3A); /* COLMOD */
    LCD_WriteData(0x05);    /* 16bit/pixel */

    LCD_WriteCommand(0x2A);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x7F);

    LCD_WriteCommand(0x2B);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x9F);

    LCD_WriteCommand(0xE0);
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

    LCD_WriteCommand(0xE1);
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

    LCD_WriteCommand(0x29); /* Display On */
    HAL_Delay(100);

    LCD_SetBacklight(1U);

    LCD_Clear(BLACK);
    LCD_Flush();
}

/* -------------------------------------------------------------------------- */
/* 基本绘图（只写帧缓冲）                                                       */
/* -------------------------------------------------------------------------- */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
    {
        return;
    }
    s_framebuffer[(uint32_t)y * LCD_WIDTH + x] = color;
}

void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t x;
    uint16_t y;

    if (x1 >= LCD_WIDTH)  { x1 = LCD_WIDTH - 1U; }
    if (y1 >= LCD_HEIGHT) { y1 = LCD_HEIGHT - 1U; }
    if (x2 >= LCD_WIDTH)  { x2 = LCD_WIDTH - 1U; }
    if (y2 >= LCD_HEIGHT) { y2 = LCD_HEIGHT - 1U; }
    if (x1 > x2) { uint16_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; y++)
    {
        uint16_t *row = &s_framebuffer[(uint32_t)y * LCD_WIDTH + x1];
        for (x = x1; x <= x2; x++)
        {
            *row++ = color;
        }
    }
}

void LCD_Clear(uint16_t color)
{
    LCD_Fill(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U, color);
}

void LCD_DrawHLine(uint16_t x1, uint16_t x2, uint16_t y, uint16_t color)
{
    LCD_Fill(x1, y, x2, y, color);
}

void LCD_DrawVLine(uint16_t x, uint16_t y1, uint16_t y2, uint16_t color)
{
    LCD_Fill(x, y1, x, y2, color);
}

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawHLine(x1, x2, y1, color);
    LCD_DrawHLine(x1, x2, y2, color);
    LCD_DrawVLine(x1, y1, y2, color);
    LCD_DrawVLine(x2, y1, y2, color);
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int32_t dx = (int32_t)x2 - (int32_t)x1;
    int32_t dy = (int32_t)y2 - (int32_t)y1;
    int32_t sx = (dx >= 0) ? 1 : -1;
    int32_t sy = (dy >= 0) ? 1 : -1;
    int32_t err;
    int32_t e2;

    if (dx < 0) { dx = -dx; }
    if (dy < 0) { dy = -dy; }
    err = dx - dy;

    for (;;)
    {
        LCD_DrawPoint(x1, y1, color);
        if ((x1 == x2) && (y1 == y2))
        {
            break;
        }
        e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 = (uint16_t)((int32_t)x1 + sx); }
        if (e2 < dx)  { err += dx; y1 = (uint16_t)((int32_t)y1 + sy); }
    }
}

/* -------------------------------------------------------------------------- */
/* 文字                                                                        */
/* -------------------------------------------------------------------------- */
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color)
{
    uint32_t i;
    uint32_t j;
    uint8_t  temp;

    /* 字库只包含可打印 ASCII，越界字符用 '?' 代替，避免数组越界读取 */
    if ((ch < ' ') || (ch > '~'))
    {
        ch = '?';
    }
    if ((font.table == NULL) || (font.width == 0U) || (font.height == 0U))
    {
        return;
    }

    for (i = 0U; i < font.height; i++)
    {
        temp = font.table[((uint32_t)ch - (uint32_t)' ') * font.height + i];
        for (j = 0U; j < font.width; j++)
        {
            if ((temp & (0x01U << j)) != 0U)
            {
                LCD_DrawPoint((uint16_t)(x + j), (uint16_t)(y + i), color);
            }
        }
    }
}

void LCD_ShowString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color)
{
    if (str == NULL)
    {
        return;
    }

    while (*str != '\0')
    {
        LCD_ShowChar(x, y, *str, font, color);
        x = (uint16_t)(x + font.width);
        str++;

        if ((uint32_t)x + font.width >= LCD_WIDTH)
        {
            x = 0U;
            y = (uint16_t)(y + font.height);
        }
    }
}

static void LCD_NumberToString(uint32_t num, char *str)
{
    uint8_t i = 0U;
    uint8_t j = 0U;
    char    temp[12];

    if (num == 0U)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while ((num > 0U) && (i < (uint8_t)sizeof(temp)))
    {
        temp[i++] = (char)('0' + (num % 10U));
        num /= 10U;
    }

    while (i > 0U)
    {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FontDef font, uint16_t color)
{
    char     str[12];
    uint8_t  num_len = 0U;
    uint32_t temp_num = num;
    const char *digits = str;

    do
    {
        num_len++;
        temp_num /= 10U;
    } while (temp_num > 0U);

    LCD_NumberToString(num, str);

    /* 位数超过显示宽度时只显示低 len 位，避免下标下溢 */
    if (num_len > len)
    {
        digits = &str[num_len - len];
        num_len = len;
    }

    for (uint8_t t = 0U; t < len; t++)
    {
        if (t < (uint8_t)(len - num_len))
        {
            LCD_ShowChar((uint16_t)(x + t * font.width), y, ' ', font, color);
        }
        else
        {
            LCD_ShowChar((uint16_t)(x + t * font.width), y, digits[t - (len - num_len)],
                         font, color);
        }
    }
}

void LCD_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t char_index, uint16_t color)
{
    uint32_t i;
    uint32_t j;

    if (char_index >= LCD_CHINESE_16X16_COUNT)
    {
        return;
    }

    for (i = 0U; i < 16U; i++)
    {
        for (j = 0U; j < 16U; j++)
        {
            uint8_t byte_index = (uint8_t)(i * 2U + (j / 8U));
            if ((Chinese_16x16[char_index][byte_index] & (0x80U >> (j % 8U))) != 0U)
            {
                LCD_DrawPoint((uint16_t)(x + j), (uint16_t)(y + i), color);
            }
        }
    }
}

void LCD_ShowChineseString16x16(uint16_t x, uint16_t y, const uint8_t *char_indexes,
                                uint8_t count, uint16_t color)
{
    if (char_indexes == NULL)
    {
        return;
    }

    for (uint8_t i = 0U; i < count; i++)
    {
        LCD_ShowChinese16x16((uint16_t)(x + i * 16U), y, char_indexes[i], color);
    }
}

void LCD_ShowBedImage(uint16_t x, uint16_t y)
{
    /*
     * gImage_bed 是 Image2Lcd 输出：8 字节文件头 + 48*48*2 像素数据 = 4616 字节。
     * 旧代码写成 +12，会越界读取 4 字节，并且整体像素错位。
     */
    const uint8_t *image_data = gImage_bed + 8;
    const uint16_t width = 48U;
    const uint16_t height = 48U;

    for (uint16_t i = 0U; i < height; i++)
    {
        for (uint16_t j = 0U; j < width; j++)
        {
            uint32_t index = ((uint32_t)i * width + j) * 2U;
            uint16_t pixel_color = (uint16_t)((image_data[index] << 8) |
                                              image_data[index + 1U]);

            if (pixel_color != 0x0000U)
            {
                LCD_DrawPoint((uint16_t)(x + j), (uint16_t)(y + i), pixel_color);
            }
        }
    }

    LCD_DrawPoint((uint16_t)(x + 47U), (uint16_t)(y + 47U), SILVER);
    LCD_DrawPoint((uint16_t)(x + 46U), (uint16_t)(y + 47U), SILVER);
}

/* -------------------------------------------------------------------------- */
/* 控制                                                                        */
/* -------------------------------------------------------------------------- */
void LCD_SetDirection(uint8_t direction)
{
    LCD_WriteCommand(0x36);
    LCD_WriteData(direction);
}

void LCD_SetBacklight(uint8_t state)
{
    s_backlight_on = (state != 0U) ? 1U : 0U;
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN,
                      (s_backlight_on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t LCD_GetBacklight(void)
{
    return s_backlight_on;
}

void LCD_DisplayOn(void)
{
    LCD_WriteCommand(0x29);
}

void LCD_DisplayOff(void)
{
    LCD_WriteCommand(0x28);
}
