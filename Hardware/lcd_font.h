#ifndef __LCD_FONT_H
#define __LCD_FONT_H

#include "stdint.h"

// 英文字符字库
extern const uint8_t ASCII_8x8[][8];
extern const uint8_t ASCII_8x16[][16];
extern const uint8_t ASCII_12x12[][24];
extern const uint8_t ASCII_16x16[][32];
extern const uint8_t ASCII_24x24[][72];
extern const uint8_t ASCII_32x32[][128];

// 中文字符字库
extern const uint8_t Chinese_16x16[][32];
extern const uint8_t Chinese_24x24[][72];
extern const uint8_t Chinese_32x32[][128];

// 数字字库
extern const uint8_t Number_8x16[][16];
extern const uint8_t Number_16x32[][64];

// 字体结构体
typedef struct {
    const uint8_t *table;
    uint16_t width;
    uint16_t height;
} FontDef;

// 字体定义
extern FontDef Font_8x8;
extern FontDef Font_8x16;
extern FontDef Font_12x12;
extern FontDef Font_16x16;
extern FontDef Font_24x24;
extern FontDef Font_32x32;

extern FontDef Font_CN_16x16;
extern FontDef Font_CN_24x24;
extern FontDef Font_CN_32x32;

extern FontDef Font_Num_8x16;
extern FontDef Font_Num_16x32;

// 颜色定义保持不变...
// 基础颜色
#define WHITE         0xFFFF
#define BLACK         0x0000
#define RED           0xF800
#define GREEN         0x07E0
#define BLUE          0x001F
#define YELLOW        0xFFE0
#define MAGENTA       0xF81F
#define CYAN          0x07FF
#define GRAY          0x8430

// 更多红色系
#define MAROON        0x8000  // 栗色
#define DARK_RED      0x8800  // 深红色
#define LIGHT_RED     0xFC10  // 浅红色
#define PINK          0xFE19  // 粉色
#define CRIMSON       0xD8A7  // 深红色

// 更多绿色系
#define DARK_GREEN    0x0320  // 深绿色
#define LIGHT_GREEN   0x07F0  // 浅绿色
#define LIME          0x87E0  // 柠檬绿
#define OLIVE         0x7BE0  // 橄榄绿
#define FOREST_GREEN  0x2444  // 森林绿

// 更多蓝色系
#define NAVY          0x0010  // 海军蓝
#define DARK_BLUE     0x0011  // 深蓝色
#define LIGHT_BLUE    0x051D  // 浅蓝色
#define SKY_BLUE      0x867D  // 天蓝色
#define ROYAL_BLUE    0x435C  // 皇家蓝

// 橙色和棕色系
#define ORANGE        0xFD20  // 橙色
#define DARK_ORANGE   0xFC00  // 深橙色
#define GOLD          0xFEA0  // 金色
#define BROWN         0xA145  // 棕色
#define SADDLE_BROWN  0x8A22  // 鞍棕色

// 紫色系
#define PURPLE        0x8010  // 紫色
#define DARK_PURPLE   0x8018  // 深紫色
#define INDIGO        0x4810  // 靛蓝色
#define VIOLET        0xEC1D  // 紫罗兰色
#define PLUM          0xDD1B  // 梅红色

// 其他颜色
#define SILVER        0xC618  // 银色
#define LIGHT_GRAY    0xD69A  // 浅灰色
#define DARK_GRAY     0x7BEF  // 深灰色
#define TURQUOISE     0x471A  // 青绿色
#define CORAL         0xFBEA  // 珊瑚色

// 暖色调
#define BEIGE         0xF7BB  // 米色
#define WHEAT         0xF6F6  // 小麦色
#define CHOCOLATE     0xD343  // 巧克力色
#define TAN           0xD5B1  // 棕褐色

extern const unsigned char gImage_bed[4616];

#endif