#include "lcd_menu.h"
#include <string.h>

// 菜单系统实例
static MenuSystem menu;

// 颜色定义 - 与主界面风格一致
#define MENU_BG_COLOR SILVER        // 银色背景，与主界面一致
#define MENU_TEXT_COLOR BLACK       // 黑色文字
#define MENU_SELECTED_BG_COLOR NAVY // 海军蓝选中背景
#define MENU_SELECTED_TEXT_COLOR WHITE // 白色选中文字
#define MENU_BORDER_COLOR DARK_GRAY // 深灰色边框
#define MENU_TITLE_COLOR DARK_BLUE  // 深蓝色标题
#define MENU_INDICATOR_COLOR GOLD   // 金色指示器

// 菜单显示参数 - 居中调整
#define MENU_WIDTH 120
#define MENU_ITEM_HEIGHT 24
#define MENU_X 4           // 左移，避免向右偏移
#define MENU_Y 4           // 上移，避免向下偏移
#define VISIBLE_ITEMS 5

// 初始化菜单系统
void Menu_Init(MenuItem* items, uint8_t count)
{
    menu.items = items;
    menu.item_count = count;
    menu.selected_index = 0;
    menu.visible_items = (count > VISIBLE_ITEMS) ? VISIBLE_ITEMS : count;
    menu.first_visible = 0;
}

// 绘制菜单
void Menu_Draw(void)
{
    uint8_t i, draw_index;
    uint16_t y_pos;
    
    // 清屏
    LCD_Clear(MENU_BG_COLOR);
    
    // 计算菜单总高度
    uint16_t menu_height = menu.visible_items * MENU_ITEM_HEIGHT;
    
    // 绘制菜单边框 - 调整到屏幕中央偏上
    LCD_DrawRectangle(MENU_X, MENU_Y, 
                     MENU_X + MENU_WIDTH, 
                     MENU_Y + menu_height, 
                     MENU_BORDER_COLOR);
    
    // 绘制菜单项
    for(i = 0; i < menu.visible_items; i++)
    {
        draw_index = menu.first_visible + i;
        if(draw_index >= menu.item_count) break;
        
        y_pos = MENU_Y + i * MENU_ITEM_HEIGHT;
        
        // 如果是选中项，绘制选中背景
        if(draw_index == menu.selected_index)
        {
            LCD_Fill(MENU_X + 1, y_pos + 1, 
                    MENU_X + MENU_WIDTH - 1, 
                    y_pos + MENU_ITEM_HEIGHT - 1, 
                    MENU_SELECTED_BG_COLOR);
            
            // 绘制选中项文本
            LCD_ShowString(MENU_X + 8, y_pos + 4, 
                          menu.items[draw_index].name, 
                          Font_8x16, MENU_SELECTED_TEXT_COLOR);
            
            // 绘制选中指示器
            LCD_Fill(MENU_X + 3, y_pos + 10, 
                    MENU_X + 5, y_pos + 14, 
                    YELLOW);
        }
        else
        {
            // 绘制普通项背景
            LCD_Fill(MENU_X + 1, y_pos + 1, 
                    MENU_X + MENU_WIDTH - 1, 
                    y_pos + MENU_ITEM_HEIGHT - 1, 
                    MENU_BG_COLOR);
            
            // 绘制普通项文本
            LCD_ShowString(MENU_X + 8, y_pos + 4, 
                          menu.items[draw_index].name, 
                          Font_8x16, MENU_TEXT_COLOR);
        }
        
        // 绘制分隔线
        if(i < menu.visible_items - 1)
        {
            LCD_Fill(MENU_X + 1, y_pos + MENU_ITEM_HEIGHT, 
                    MENU_X + MENU_WIDTH - 1, 
                    y_pos + MENU_ITEM_HEIGHT, 
                    MENU_BORDER_COLOR);
        }
    }
    
    // 如果菜单项超过可见数量，显示滚动指示器
    if(menu.item_count > menu.visible_items)
    {
        // 顶部滚动指示器
        if(menu.first_visible > 0)
        {
            uint16_t indicator_x = MENU_X + MENU_WIDTH - 10;
            uint16_t indicator_y = MENU_Y + 2;
            
            // 简单的点状指示器
            LCD_Fill(indicator_x, indicator_y, 
                    indicator_x + 2, indicator_y + 2, 
                    GRAY);
        }
        
        // 底部滚动指示器
        if(menu.first_visible + menu.visible_items < menu.item_count)
        {
            uint16_t indicator_x = MENU_X + MENU_WIDTH - 10;
            uint16_t indicator_y = MENU_Y + menu_height - 4;
            
            LCD_Fill(indicator_x, indicator_y, 
                    indicator_x + 2, indicator_y + 2, 
                    GRAY);
        }
    }
    
    // 绘制菜单标题 - 放在菜单上方
    //LCD_ShowString(MENU_X + 20, 0, "MENU", Font_8x16, CYAN);
}

// 向上移动选择
void Menu_Up(void)
{
    if(menu.selected_index > 0)
    {
        menu.selected_index--;
        
        // 如果选中项不在可见区域，调整可见区域
        if(menu.selected_index < menu.first_visible)
        {
            menu.first_visible = menu.selected_index;
        }
        
        Menu_Draw();
    }
}

// 向下移动选择
void Menu_Down(void)
{
    if(menu.selected_index < menu.item_count - 1)
    {
        menu.selected_index++;
        
        // 如果选中项不在可见区域，调整可见区域
        if(menu.selected_index >= menu.first_visible + menu.visible_items)
        {
            menu.first_visible = menu.selected_index - menu.visible_items + 1;
        }
        
        Menu_Draw();
    }
}

// 选择当前菜单项
void Menu_Select(void)
{
    if(menu.selected_index < menu.item_count && menu.items[menu.selected_index].callback)
    {
        menu.items[menu.selected_index].callback(menu.selected_index);
    }
}

// 获取当前选中项索引
uint8_t Menu_GetSelectedIndex(void)
{
    return menu.selected_index;
}