#include "ui_views.h"
#include <string.h>

/* 中文字模索引（与 lcd_font.c 中的字模顺序一致） */
static const uint8_t s_cn_welcome[]   = {0U, 1U, 2U, 3U};              /* 欢迎来到 */
static const uint8_t s_cn_dorm[]      = {4U, 5U, 6U};                  /* 榕华二   */
static const uint8_t s_cn_locked[]    = {7U, 8U, 9U};                  /* 已上锁   */
static const uint8_t s_cn_status[]    = {10U, 11U, 12U};               /* 状态:    */
static const uint8_t s_cn_input_pwd[] = {13U, 14U, 15U, 16U, 17U};     /* 请输入密码 */
static const uint8_t s_cn_pwd[]       = {16U, 17U};                    /* 密码     */
static const uint8_t s_cn_pwd_ok[]    = {39U, 40U, 41U, 42U};          /* 密码正确 */
static const uint8_t s_cn_welcome_back[] = {0U, 1U, 27U, 29U};        /* 欢迎回宿 */
static const uint8_t s_cn_auto_back[] = {20U, 21U, 22U, 23U, 24U, 25U, 26U}; /* 自动回到主界面 */
static const uint8_t s_cn_new_pwd[]   = {13U, 14U, 15U, 31U, 16U, 17U}; /* 请输入新密码 */
static const uint8_t s_cn_locked_out[] = {36U, 37U, 38U};              /* 已锁机   */

static const char *const s_menu_labels[] = {
    "Reset PWD",
    "Register FP",
    "Clear all FP"
};
#define UI_MENU_COUNT ((uint8_t)(sizeof(s_menu_labels) / sizeof(s_menu_labels[0])))

/* 密码框区域 */
#define UI_PWD_BOX_X1 50U
#define UI_PWD_BOX_Y1 43U
#define UI_PWD_BOX_X2 114U
#define UI_PWD_BOX_Y2 63U
#define UI_PWD_DOT_X  55U
#define UI_PWD_DOT_Y  45U
#define UI_PWD_DOT_STEP 8U
#define UI_PWD_DOT_MAX 7U

static uint16_t text_x_center(const char *text, uint16_t char_width)
{
    uint16_t length;
    uint16_t width;

    if (text == NULL)
    {
        return 0U;
    }

    length = (uint16_t)strlen(text);
    width = (uint16_t)(length * char_width);
    if (width >= LCD_WIDTH)
    {
        return 0U;
    }
    return (uint16_t)((LCD_WIDTH - width) / 2U);
}

static void draw_password_box(void)
{
    LCD_DrawRectangle(UI_PWD_BOX_X1, UI_PWD_BOX_Y1, UI_PWD_BOX_X2, UI_PWD_BOX_Y2, BLACK);
    LCD_Fill((uint16_t)(UI_PWD_BOX_X1 + 1U), (uint16_t)(UI_PWD_BOX_Y1 + 1U),
             (uint16_t)(UI_PWD_BOX_X2 - 1U), (uint16_t)(UI_PWD_BOX_Y2 - 1U), WHITE);
}

static void draw_password_dots(uint8_t dots)
{
    uint16_t x = UI_PWD_DOT_X;
    uint8_t  count = (dots > UI_PWD_DOT_MAX) ? UI_PWD_DOT_MAX : dots;

    LCD_Fill((uint16_t)(UI_PWD_BOX_X1 + 1U), (uint16_t)(UI_PWD_BOX_Y1 + 1U),
             (uint16_t)(UI_PWD_BOX_X2 - 1U), (uint16_t)(UI_PWD_BOX_Y2 - 1U), WHITE);

    for (uint8_t i = 0U; i < count; i++)
    {
        LCD_ShowString(x, UI_PWD_DOT_Y, "*", Font_8x16, BLACK);
        x = (uint16_t)(x + UI_PWD_DOT_STEP);
    }
}

/* -------------------------------------------------------------------------- */
/* 主界面                                                                      */
/* -------------------------------------------------------------------------- */
void UI_ShowMain(void)
{
    LCD_Clear(SILVER);

    LCD_ShowChineseString16x16(10U, 10U, s_cn_welcome, (uint8_t)sizeof(s_cn_welcome), BLACK);
    LCD_ShowChineseString16x16(42U, 30U, s_cn_dorm, (uint8_t)sizeof(s_cn_dorm), BLACK);
    LCD_ShowString(90U, 30U, "1115", Font_8x16, BLACK);

    LCD_DrawRectangle(0U, 50U, LCD_WIDTH - 1U, 50U, GRAY);

    LCD_ShowChineseString16x16(16U, 60U, s_cn_status, (uint8_t)sizeof(s_cn_status), BLACK);
    LCD_ShowChineseString16x16(60U, 60U, s_cn_locked, (uint8_t)sizeof(s_cn_locked), RED);

    /* 宿舍楼 */
    LCD_DrawRectangle(30U, 100U, 90U, 159U, BLUE);
    LCD_Fill(31U, 101U, 89U, 158U, BLUE);

    LCD_DrawRectangle(30U, 120U, 90U, 120U, WHITE);
    LCD_DrawRectangle(30U, 140U, 90U, 140U, WHITE);

    /* 三楼窗户 */
    LCD_DrawRectangle(40U, 105U, 50U, 115U, YELLOW);
    LCD_Fill(41U, 106U, 49U, 114U, YELLOW);
    LCD_DrawRectangle(70U, 105U, 80U, 115U, YELLOW);
    LCD_Fill(71U, 106U, 79U, 114U, YELLOW);

    /* 二楼窗户 */
    LCD_DrawRectangle(40U, 125U, 50U, 135U, YELLOW);
    LCD_Fill(41U, 126U, 49U, 134U, YELLOW);
    LCD_DrawRectangle(70U, 125U, 80U, 135U, YELLOW);
    LCD_Fill(71U, 126U, 79U, 134U, YELLOW);

    /* 一楼窗户 */
    LCD_DrawRectangle(40U, 145U, 50U, 155U, YELLOW);
    LCD_Fill(41U, 146U, 49U, 154U, YELLOW);
    LCD_DrawRectangle(70U, 145U, 80U, 155U, YELLOW);
    LCD_Fill(71U, 146U, 79U, 154U, YELLOW);

    /* 宿舍门 */
    LCD_DrawRectangle(55U, 145U, 65U, 159U, BROWN);
    LCD_Fill(56U, 146U, 64U, 158U, BROWN);

    LCD_Flush();
}

/* -------------------------------------------------------------------------- */
/* 密码输入                                                                    */
/* -------------------------------------------------------------------------- */
void UI_ShowInput(UI_InputMode mode, uint8_t dots)
{
    LCD_Clear(SILVER);

    if (mode == UI_INPUT_NEW)
    {
        LCD_ShowChineseString16x16(10U, 10U, s_cn_new_pwd,
                                   (uint8_t)sizeof(s_cn_new_pwd), BLACK);
    }
    else if (mode == UI_INPUT_ADMIN)
    {
        LCD_ShowString(10U, 10U, "Admin password", Font_8x16, BLACK);
    }
    else
    {
        LCD_ShowChineseString16x16(10U, 10U, s_cn_input_pwd,
                                   (uint8_t)sizeof(s_cn_input_pwd), BLACK);
    }

    LCD_DrawRectangle(0U, 35U, LCD_WIDTH - 1U, 35U, GRAY);
    LCD_ShowChineseString16x16(10U, 45U, s_cn_pwd, (uint8_t)sizeof(s_cn_pwd), BLACK);

    draw_password_box();
    draw_password_dots(dots);

    LCD_Flush();
}

void UI_ShowInputDots(uint8_t dots)
{
    draw_password_dots(dots);
    LCD_FlushArea(UI_PWD_BOX_X1, UI_PWD_BOX_Y1, UI_PWD_BOX_X2, UI_PWD_BOX_Y2);
}

/* -------------------------------------------------------------------------- */
/* 开门成功                                                                    */
/* -------------------------------------------------------------------------- */
void UI_ShowSuccess(void)
{
    LCD_Clear(SILVER);

    LCD_ShowChineseString16x16(32U, 15U, s_cn_pwd_ok, (uint8_t)sizeof(s_cn_pwd_ok), GREEN);
    LCD_DrawRectangle(10U, 40U, 118U, 40U, GRAY);
    LCD_ShowChineseString16x16(32U, 50U, s_cn_welcome_back,
                               (uint8_t)sizeof(s_cn_welcome_back), BLACK);
    LCD_ShowBedImage(36U, 75U);
    LCD_ShowChineseString16x16(10U, 130U, s_cn_auto_back,
                               (uint8_t)sizeof(s_cn_auto_back), DARK_GRAY);

    LCD_Flush();
}

/* -------------------------------------------------------------------------- */
/* 锁定                                                                        */
/* -------------------------------------------------------------------------- */
void UI_ShowLocked(uint32_t remaining_seconds)
{
    LCD_Clear(SILVER);

    LCD_ShowChineseString16x16(40U, 15U, s_cn_locked_out,
                               (uint8_t)sizeof(s_cn_locked_out), RED);
    LCD_DrawRectangle(10U, 40U, 118U, 40U, GRAY);

    LCD_ShowString(text_x_center("Please wait", 8U), 55U, "Please wait",
                   Font_8x16, BLACK);
    LCD_ShowNum(40U, 78U, remaining_seconds, 4U, Font_8x16, RED);
    LCD_ShowString(80U, 78U, "sec", Font_8x16, BLACK);

    LCD_ShowBedImage(40U, 105U);

    LCD_Flush();
}

/*
 * 锁机倒计时只刷新数字所在的小区域：
 *   x=40..71, y=78..93（4 位 8x16 数字），四周各留 1px 余量。
 * 不重画标题、分隔线、Please wait、床图，避免整屏闪烁和慢刷新。
 */
void UI_UpdateLockedCountdown(uint32_t remaining_seconds)
{
    LCD_Fill(39U, 77U, 72U, 94U, SILVER);
    LCD_ShowNum(40U, 78U, remaining_seconds, 4U, Font_8x16, RED);
}

/* -------------------------------------------------------------------------- */
/* 管理员菜单                                                                  */
/* -------------------------------------------------------------------------- */
#define UI_MENU_X      4U
#define UI_MENU_Y      32U
#define UI_MENU_WIDTH  120U
#define UI_MENU_ITEM_H 30U

void UI_ShowMenu(uint8_t selected)
{
    uint16_t y;

    LCD_Clear(SILVER);
    LCD_ShowString(text_x_center("Admin Menu", 8U), 8U, "Admin Menu",
                   Font_8x16, DARK_BLUE);

    LCD_DrawRectangle(UI_MENU_X, UI_MENU_Y,
                      (uint16_t)(UI_MENU_X + UI_MENU_WIDTH),
                      (uint16_t)(UI_MENU_Y + UI_MENU_COUNT * UI_MENU_ITEM_H),
                      DARK_GRAY);

    for (uint8_t i = 0U; i < UI_MENU_COUNT; i++)
    {
        uint16_t item_y = (uint16_t)(UI_MENU_Y + i * UI_MENU_ITEM_H);

        if (i == selected)
        {
            LCD_Fill((uint16_t)(UI_MENU_X + 1U), (uint16_t)(item_y + 1U),
                     (uint16_t)(UI_MENU_X + UI_MENU_WIDTH - 1U),
                     (uint16_t)(item_y + UI_MENU_ITEM_H - 1U), NAVY);
            LCD_ShowString((uint16_t)(UI_MENU_X + 12U), (uint16_t)(item_y + 7U),
                           s_menu_labels[i], Font_8x16, WHITE);
            LCD_Fill((uint16_t)(UI_MENU_X + 4U), (uint16_t)(item_y + 12U),
                     (uint16_t)(UI_MENU_X + 6U), (uint16_t)(item_y + 16U), YELLOW);
        }
        else
        {
            LCD_ShowString((uint16_t)(UI_MENU_X + 12U), (uint16_t)(item_y + 7U),
                           s_menu_labels[i], Font_8x16, BLACK);
        }

        if (i + 1U < UI_MENU_COUNT)
        {
            y = (uint16_t)(item_y + UI_MENU_ITEM_H);
            LCD_DrawHLine((uint16_t)(UI_MENU_X + 1U),
                          (uint16_t)(UI_MENU_X + UI_MENU_WIDTH - 1U), y, GRAY);
        }
    }

    LCD_Flush();
}

uint8_t UI_MenuItemCount(void)
{
    return UI_MENU_COUNT;
}

const char *UI_MenuItemLabel(uint8_t index)
{
    if (index >= UI_MENU_COUNT)
    {
        return "";
    }
    return s_menu_labels[index];
}

/* -------------------------------------------------------------------------- */
/* 指纹注册提示                                                                */
/* -------------------------------------------------------------------------- */
void UI_ShowEnrollPrompt(void)
{
    LCD_Clear(SILVER);

    LCD_ShowString(text_x_center("Register FP", 8U), 10U, "Register FP",
                   Font_8x16, DARK_BLUE);
    LCD_DrawRectangle(10U, 36U, 118U, 36U, GRAY);

    LCD_ShowString(text_x_center("Put finger", 8U), 60U,
                   "Put finger", Font_8x16, BLACK);
    LCD_ShowString(text_x_center("on sensor", 8U), 82U,
                   "on sensor", Font_8x16, BLACK);

    LCD_ShowBedImage(40U, 105U);

    LCD_Flush();
}

/* -------------------------------------------------------------------------- */
/* 通用提示                                                                    */
/* -------------------------------------------------------------------------- */
void UI_ShowMessage(const char *line1, const char *line2, uint16_t color)
{
    LCD_Clear(SILVER);

    LCD_DrawRectangle(10U, 20U, 118U, 20U, GRAY);

    if (line1 != NULL)
    {
        LCD_ShowString(text_x_center(line1, 8U), 55U, line1, Font_8x16, color);
    }
    if (line2 != NULL)
    {
        LCD_ShowString(text_x_center(line2, 8U), 80U, line2, Font_8x16, DARK_GRAY);
    }

    LCD_ShowBedImage(40U, 105U);
    LCD_Flush();
}
