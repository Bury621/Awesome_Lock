#ifndef __LCD_MENU_H
#define __LCD_MENU_H

#include "lcd.h"
#include "lcd_font.h"

// 菜单项回调函数类型定义
typedef void (*MenuCallback)(uint8_t selected_item);

// 菜单结构体
typedef struct {
    const char* name;        // 菜单项名称
    MenuCallback callback;   // 回调函数
} MenuItem;

// 菜单系统结构体
typedef struct {
    MenuItem* items;         // 菜单项数组
    uint8_t item_count;      // 菜单项数量
    uint8_t selected_index;  // 当前选中项索引
    uint8_t visible_items;   // 可见菜单项数量
    uint8_t first_visible;   // 第一个可见菜单项索引
} MenuSystem;

// 函数声明
void Menu_Init(MenuItem* items, uint8_t count);
void Menu_Draw(void);
void Menu_Up(void);
void Menu_Down(void);
void Menu_Select(void);
uint8_t Menu_GetSelectedIndex(void);

#endif