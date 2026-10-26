#include "lock_app.h"
#include "app_util.h"
#include "ui_views.h"
#include "Keyboard.h"
#include "stepper_motor.h"
#include "flash_eeprom.h"
#include "Fingerprint.h"
#include <stdio.h>
#include <string.h>

/* 电机方向：按实际装配方向选择 */
#ifdef MOTOR_REVERSE
#define DOOR_OPEN_DIR   MOTOR_CCW
#define DOOR_CLOSE_DIR  MOTOR_CW
#else
#define DOOR_OPEN_DIR   MOTOR_CW
#define DOOR_CLOSE_DIR  MOTOR_CCW
#endif

typedef enum
{
    LOCK_STATE_MAIN = 0,
    LOCK_STATE_INPUT,
    LOCK_STATE_UNLOCKED,
    LOCK_STATE_ADMIN_CHECK,
    LOCK_STATE_ADMIN_MENU,
    LOCK_STATE_ADMIN_NEW_PWD,
    LOCK_STATE_ADMIN_ENROLL,
    LOCK_STATE_ADMIN_FP_BUSY,
    LOCK_STATE_MESSAGE,
    LOCK_STATE_LOCKED
} LockState;

static LockState s_state;
static char      s_pwd[PWD_MAX_LEN + 1U];
static uint8_t   s_pwd_len;

static uint8_t  s_pwd_fail;
static uint8_t  s_fp_fail;
static uint32_t s_lockout_until;
static uint32_t s_lockout_tick;

static uint32_t s_last_activity;
static bool     s_sleeping;

static bool     s_keypad_pending;
static uint32_t s_keypad_due;

static bool     s_door_open;
static bool     s_door_hold;
static bool     s_close_requested;
static bool     s_open_requested;
static uint32_t s_door_close_at;
static uint32_t s_auto_return_at;

static uint32_t s_light_restore_at;

static bool     s_fp_retry_armed;
static uint32_t s_fp_retry_at;

static uint8_t   s_menu_index;
static LockState s_message_return;
static uint32_t  s_message_deadline;

/* -------------------------------------------------------------------------- */
/* 前置声明                                                                    */
/* -------------------------------------------------------------------------- */
static void enter_main(void);
static void enter_input(LockState state, UI_InputMode mode);
static void enter_admin_menu(uint8_t index);
static void enter_after_message(void);
static void show_message(const char *line1, const char *line2, uint16_t color,
                         LockState return_state, uint32_t duration_ms);
static void unlock_success(void);
static void lockout_enter(void);
static void door_open(void);
static void door_close(void);
static void on_fp_event(FPC_Op op, FPC_Result result, FGP_Status_Code status);

/* -------------------------------------------------------------------------- */
/* 密码缓冲                                                                    */
/* -------------------------------------------------------------------------- */
static void pwd_clear(void)
{
    (void)memset(s_pwd, 0, sizeof(s_pwd));
    s_pwd_len = 0U;
}

static bool pwd_append(char ch)
{
    if (s_pwd_len >= (PWD_MAX_LEN - 1U))
    {
        return false;
    }

    s_pwd[s_pwd_len++] = ch;
    s_pwd[s_pwd_len] = '\0';
    return true;
}

static void pwd_pop(void)
{
    if (s_pwd_len > 0U)
    {
        s_pwd_len--;
        s_pwd[s_pwd_len] = '\0';
    }
}

static bool pwd_equals(const char *expected)
{
    return (expected != NULL) && (strcmp(s_pwd, expected) == 0);
}

static bool is_digit_key(char key)
{
    return (key >= '0') && (key <= '9');
}

/* -------------------------------------------------------------------------- */
/* 界面切换                                                                    */
/* -------------------------------------------------------------------------- */
static void enter_main(void)
{
    pwd_clear();
    s_state = LOCK_STATE_MAIN;
    s_door_close_at = 0U;
    s_auto_return_at = 0U;
    UI_ShowMain();
    FPC_SetLight((s_fp_fail >= FGP_MAX_VERIFY) ? FPC_LIGHT_RED_CONST
                                               : FPC_LIGHT_BLUE_BREATH);
    s_light_restore_at = 0U;
    s_last_activity = HAL_GetTick();
}

static void enter_input(LockState state, UI_InputMode mode)
{
    pwd_clear();
    s_state = state;
    UI_ShowInput(mode, 0U);
}

static void enter_admin_menu(uint8_t index)
{
    pwd_clear();
    s_menu_index = index;
    s_state = LOCK_STATE_ADMIN_MENU;
    UI_ShowMenu(index);
}

static void show_message(const char *line1, const char *line2, uint16_t color,
                         LockState return_state, uint32_t duration_ms)
{
    s_state = LOCK_STATE_MESSAGE;
    s_message_return = return_state;
    s_message_deadline = HAL_GetTick() + duration_ms;
    UI_ShowMessage(line1, line2, color);
}

static void enter_after_message(void)
{
    switch (s_message_return)
    {
        case LOCK_STATE_MAIN:
            enter_main();
            break;
        case LOCK_STATE_INPUT:
            enter_input(LOCK_STATE_INPUT, UI_INPUT_PWD);
            break;
        case LOCK_STATE_ADMIN_CHECK:
            enter_input(LOCK_STATE_ADMIN_CHECK, UI_INPUT_ADMIN);
            break;
        case LOCK_STATE_ADMIN_NEW_PWD:
            enter_input(LOCK_STATE_ADMIN_NEW_PWD, UI_INPUT_NEW);
            break;
        case LOCK_STATE_ADMIN_MENU:
            enter_admin_menu(s_menu_index);
            break;
        default:
            enter_main();
            break;
    }
}

/* -------------------------------------------------------------------------- */
/* 门 / 电机                                                                   */
/* -------------------------------------------------------------------------- */
static void door_open(void)
{
    if (s_door_open)
    {
        return;
    }

    if (StepperMotor_IsRunning())
    {
        s_open_requested = true;
        return;
    }

    s_open_requested = false;
    s_close_requested = false;
    s_door_open = true;
    s_door_hold = false;

    StepperMotor_RotateAngle(DOOR_OPEN_DIR, (float)OPEN_DOOR_ANGLE, OPEN_DOOR_STEP_US);
    HAL_GPIO_WritePin(GPIOA, Led2_Pin, GPIO_PIN_RESET); /* 开门指示灯亮 */
}

static void door_close(void)
{
    if (!s_door_open)
    {
        s_close_requested = false;
        return;
    }

    if (StepperMotor_IsRunning())
    {
        s_close_requested = true;
        return;
    }

    s_open_requested = false;
    s_close_requested = false;
    s_door_hold = false;
    s_door_open = false;

    HAL_GPIO_WritePin(GPIOA, Led2_Pin, GPIO_PIN_SET); /* 开门指示灯灭 */
    StepperMotor_RotateAngle(DOOR_CLOSE_DIR, (float)OPEN_DOOR_ANGLE, OPEN_DOOR_STEP_US);
}

/* -------------------------------------------------------------------------- */
/* 解锁 / 锁定                                                                 */
/* -------------------------------------------------------------------------- */
static void unlock_success(void)
{
    uint32_t now = HAL_GetTick();

    s_pwd_fail = 0U;
    s_fp_fail = 0U;
    pwd_clear();

    s_state = LOCK_STATE_UNLOCKED;
    UI_ShowSuccess();
    door_open();

    s_door_close_at = now + OPEN_DOOR_TIME_MS;
    s_auto_return_at = now + OPEN_DOOR_HOLD_MS;
    s_last_activity = now;
    s_sleeping = false;
    LCD_SetBacklight(1U);

    FPC_SetLight(FPC_LIGHT_GREEN_CONST);
    s_light_restore_at = now + 1200U;
}

static void lockout_enter(void)
{
    uint32_t now = HAL_GetTick();

    s_state = LOCK_STATE_LOCKED;
    s_pwd_fail = 0U;
    s_fp_fail = 0U;
    pwd_clear();

    s_lockout_until = now + FAIL_LOCK_TIME_MS;
    s_lockout_tick = now;
    s_door_close_at = 0U;
    s_auto_return_at = 0U;

    door_close();
    FPC_SetLight(FPC_LIGHT_RED_CONST);
    s_light_restore_at = 0U;

    s_sleeping = false;
    LCD_SetBacklight(1U);
    UI_ShowLocked(FAIL_LOCK_TIME);
}

static void password_failed(LockState return_state)
{
    s_pwd_fail++;
    pwd_clear();

    if (s_pwd_fail >= FAIL_LOCK_COUNT)
    {
        lockout_enter();
    }
    else
    {
        show_message("Wrong password", "", RED, return_state, UI_MESSAGE_TIME_MS);
    }
}

/* -------------------------------------------------------------------------- */
/* 指纹                                                                        */
/* -------------------------------------------------------------------------- */
static void fp_touch(void)
{
    uint32_t now = HAL_GetTick();

    if ((s_state != LOCK_STATE_MAIN) && (s_state != LOCK_STATE_INPUT))
    {
        return;
    }

    if (s_fp_fail >= FGP_MAX_VERIFY)
    {
        /* 指纹失败次数过多：暂时禁用指纹，改用密码 */
        FPC_SetLight(FPC_LIGHT_RED_CONST);
        s_light_restore_at = now + 600U;
        return;
    }

    if (FPC_IsBusy())
    {
        return;
    }

    if (!FPC_StartVerify())
    {
        /*
         * 刚发过灯光指令时 FPC 会拒绝立即启动（避免 ACK 串包）。
         * 短暂延迟后自动重试一次，不丢用户的触摸。
         */
        s_fp_retry_armed = true;
        s_fp_retry_at = now + 200U;
    }
}

static void on_fp_event(FPC_Op op, FPC_Result result, FGP_Status_Code status)
{
    uint32_t now = HAL_GetTick();

    if (op == FPC_OP_VERIFY)
    {
        /* 用户可能已经返回主界面/进入管理员界面，此时结果作废 */
        if ((s_state != LOCK_STATE_MAIN) && (s_state != LOCK_STATE_INPUT))
        {
            return;
        }

        if (result == FPC_RESULT_OK)
        {
            unlock_success();
        }
        else if (result == FPC_RESULT_FAIL)
        {
            s_fp_fail++;
            FPC_SetLight(FPC_LIGHT_RED_CONST);
            s_light_restore_at = now + 800U;

            if (s_fp_fail >= FGP_MAX_VERIFY)
            {
                show_message("FP disabled", "Use password", RED,
                             LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
            }
        }
        else
        {
            /* 超时/通信失败：不惩罚用户，保持当前界面 */
        }
        return;
    }

    if (op == FPC_OP_ENROLL)
    {
        if (s_state != LOCK_STATE_ADMIN_ENROLL)
        {
            return;
        }

        if (result == FPC_RESULT_OK)
        {
            show_message("FP added", "", GREEN, LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
        }
        else if (result == FPC_RESULT_TIMEOUT)
        {
            show_message("Timeout", "Try again", RED, LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
        }
        else
        {
            char text[28];
            (void)snprintf(text, sizeof(text), "FP error 0x%02X", (unsigned)status);
            show_message(text, "", RED, LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
        }

        FPC_SetLight(FPC_LIGHT_BLUE_BREATH);
        s_light_restore_at = 0U;
        return;
    }

    if (op == FPC_OP_CLEAR)
    {
        if (s_state != LOCK_STATE_ADMIN_FP_BUSY)
        {
            return;
        }

        if (result == FPC_RESULT_OK)
        {
            show_message("FP cleared", "", GREEN, LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
        }
        else
        {
            show_message("Clear failed", "", RED, LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
        }

        FPC_SetLight(FPC_LIGHT_BLUE_BREATH);
        s_light_restore_at = 0U;
    }
}

/* -------------------------------------------------------------------------- */
/* 按键处理                                                                    */
/* -------------------------------------------------------------------------- */
static void admin_menu_select(void)
{
    switch (s_menu_index)
    {
        case 0U:
            enter_input(LOCK_STATE_ADMIN_NEW_PWD, UI_INPUT_NEW);
            break;

        case 1U:
            s_state = LOCK_STATE_ADMIN_ENROLL;
            pwd_clear();
            UI_ShowEnrollPrompt();
            if (!FPC_StartEnroll())
            {
                show_message("FP busy", "", RED, LOCK_STATE_MAIN, UI_MESSAGE_TIME_MS);
            }
            break;

        case 2U:
            if (FPC_StartClear())
            {
                s_state = LOCK_STATE_ADMIN_FP_BUSY;
                UI_ShowMessage("Clearing...", "", DARK_GRAY);
            }
            else
            {
                show_message("FP busy", "", RED, LOCK_STATE_ADMIN_MENU, UI_MESSAGE_TIME_MS);
            }
            break;

        default:
            break;
    }
}

static void handle_key(char key)
{
    switch (s_state)
    {
        case LOCK_STATE_MAIN:
            if (is_digit_key(key))
            {
                enter_input(LOCK_STATE_INPUT, UI_INPUT_PWD);
                (void)pwd_append(key);
                UI_ShowInputDots(s_pwd_len);
            }
            else if (key == 'A')
            {
                enter_input(LOCK_STATE_ADMIN_CHECK, UI_INPUT_ADMIN);
            }
            break;

        case LOCK_STATE_INPUT:
            if (is_digit_key(key))
            {
                if (pwd_append(key))
                {
                    UI_ShowInputDots(s_pwd_len);
                }
            }
            else if (key == '*')
            {
                pwd_pop();
                UI_ShowInputDots(s_pwd_len);
            }
            else if (key == '#')
            {
                if (s_pwd_len > 0U)
                {
                    if (pwd_equals(FLASH_EEPROM_GetPassword()))
                    {
                        unlock_success();
                    }
                    else
                    {
                        password_failed(LOCK_STATE_INPUT);
                    }
                }
            }
            else if (key == 'D')
            {
                enter_main();
            }
            break;

        case LOCK_STATE_ADMIN_CHECK:
            if (is_digit_key(key))
            {
                if (pwd_append(key))
                {
                    UI_ShowInputDots(s_pwd_len);
                }
            }
            else if (key == '*')
            {
                pwd_pop();
                UI_ShowInputDots(s_pwd_len);
            }
            else if (key == '#')
            {
                if (s_pwd_len > 0U)
                {
                    if (pwd_equals(ADMIN_PWD))
                    {
                        s_pwd_fail = 0U;
                        enter_admin_menu(0U);
                    }
                    else
                    {
                        password_failed(LOCK_STATE_ADMIN_CHECK);
                    }
                }
            }
            else if (key == 'D')
            {
                enter_main();
            }
            break;

        case LOCK_STATE_ADMIN_NEW_PWD:
            if (is_digit_key(key))
            {
                if (pwd_append(key))
                {
                    UI_ShowInputDots(s_pwd_len);
                }
            }
            else if (key == '*')
            {
                pwd_pop();
                UI_ShowInputDots(s_pwd_len);
            }
            else if (key == '#')
            {
                if (s_pwd_len < PWD_MIN_LEN)
                {
                    show_message("Too short", "Min 4 digits", RED,
                                 LOCK_STATE_ADMIN_NEW_PWD, UI_MESSAGE_TIME_MS);
                }
                else if (FLASH_EEPROM_SetPassword(s_pwd))
                {
                    show_message("Password saved", "", GREEN,
                                 LOCK_STATE_MAIN, UI_RESULT_TIME_MS);
                }
                else
                {
                    show_message("Save failed", "", RED,
                                 LOCK_STATE_ADMIN_NEW_PWD, UI_RESULT_TIME_MS);
                }
                pwd_clear();
            }
            else if (key == 'D')
            {
                enter_admin_menu(s_menu_index);
            }
            break;

        case LOCK_STATE_ADMIN_MENU:
            if (key == 'A')
            {
                if (s_menu_index > 0U)
                {
                    s_menu_index--;
                    UI_ShowMenu(s_menu_index);
                }
            }
            else if (key == 'B')
            {
                if ((uint8_t)(s_menu_index + 1U) < UI_MenuItemCount())
                {
                    s_menu_index++;
                    UI_ShowMenu(s_menu_index);
                }
            }
            else if (key == 'C')
            {
                admin_menu_select();
            }
            else if (key == 'D')
            {
                enter_main();
            }
            break;

        case LOCK_STATE_ADMIN_ENROLL:
            /*
             * 注册过程中不接受返回键：一旦取消，模块之后迟到的应答包
             * 可能被下一次验证误判（协议确认包不带指令号）。
             * 最长等待 FGP_ENROLL_TIMEOUT_MS（20s）后会自动超时返回。
             */
            break;

        case LOCK_STATE_ADMIN_FP_BUSY:
            /* 同理，清空指纹库期间等待结果 */
            break;

        case LOCK_STATE_UNLOCKED:
            if (key == 'D')
            {
                s_door_close_at = 0U;
                s_auto_return_at = 0U;
                door_close();
                enter_main();
            }
            break;

        case LOCK_STATE_MESSAGE:
            if (key == 'D')
            {
                enter_after_message();
            }
            break;

        case LOCK_STATE_LOCKED:
        default:
            break;
    }
}

/* -------------------------------------------------------------------------- */
/* 对外接口                                                                    */
/* -------------------------------------------------------------------------- */
void LockApp_Init(void)
{
    pwd_clear();

    s_state = LOCK_STATE_MAIN;
    s_pwd_fail = 0U;
    s_fp_fail = 0U;
    s_lockout_until = 0U;
    s_lockout_tick = 0U;
    s_keypad_pending = false;
    s_sleeping = false;
    s_door_open = false;
    s_door_hold = false;
    s_close_requested = false;
    s_open_requested = false;
    s_door_close_at = 0U;
    s_auto_return_at = 0U;
    s_light_restore_at = 0U;
    s_fp_retry_armed = false;
    s_fp_retry_at = 0U;
    s_menu_index = 0U;

    HAL_GPIO_WritePin(GPIOA, Led1_Pin | Led2_Pin, GPIO_PIN_SET);

    FLASH_EEPROM_Init();

    UI_ShowMain();
    LCD_SetBacklight(1U);

    FPC_Init(on_fp_event);
    FPC_SetLight(FPC_LIGHT_BLUE_BREATH);

    s_last_activity = HAL_GetTick();
}

void LockApp_HandleEvent(const AppEvent *event)
{
    uint32_t now;

    if (event == NULL)
    {
        return;
    }

    now = HAL_GetTick();

    if (s_sleeping)
    {
        /*
         * 熄屏状态下，第一个事件只负责唤醒，不参与输入，避免误触。
         * 按键事件需要真正读一次 I2C：按键模块的 IRQ 可能是“锁存到读取为止”，
         * 不读的话它不会释放，下一次按键就不会再产生下降沿。
         */
        if (event->type == APP_EVENT_KEYPAD)
        {
            uint16_t discarded_key;
            (void)Keyboard_ReadRaw(&discarded_key);
        }

        if ((event->type == APP_EVENT_KEYPAD) ||
            (event->type == APP_EVENT_FINGERPRINT) ||
            (event->type == APP_EVENT_BODY))
        {
            s_sleeping = false;
            LCD_SetBacklight(1U);
            s_last_activity = now;
            return;
        }
    }

    s_last_activity = now;

    switch (event->type)
    {
        case APP_EVENT_KEYPAD:
            if (!s_keypad_pending)
            {
                s_keypad_pending = true;
                s_keypad_due = now + KEYPAD_DEBOUNCE_MS;
            }
            break;

        case APP_EVENT_FINGERPRINT:
            fp_touch();
            break;

        case APP_EVENT_BODY:
        case APP_EVENT_DOOR_SENSE:
        default:
            break;
    }
}

void LockApp_Poll(void)
{
    uint32_t now = HAL_GetTick();

    /* 1. 按键去抖后读取（I2C 只在主循环里访问） */
    if (s_keypad_pending && App_TimeReached(s_keypad_due))
    {
        uint16_t raw_key = 0U;
        char key;

        s_keypad_pending = false;

        if (Keyboard_ReadRaw(&raw_key))
        {
            key = Keyboard_GetChar(raw_key);
            if (key != '\0')
            {
                s_last_activity = now;
                handle_key(key);
            }
        }
        else
        {
            /* 读失败时短暂重试，不阻塞主循环 */
            s_keypad_pending = true;
            s_keypad_due = now + 50U;
        }
    }

    /* 2. 指纹验证延迟重试（刚发过灯光指令时会短暂拒绝启动） */
    if (s_fp_retry_armed && App_TimeReached(s_fp_retry_at))
    {
        s_fp_retry_armed = false;
        if (((s_state == LOCK_STATE_MAIN) || (s_state == LOCK_STATE_INPUT)) &&
            (s_fp_fail < FGP_MAX_VERIFY) && !FPC_IsBusy())
        {
            (void)FPC_StartVerify();
        }
    }

    /* 3. 指纹灯恢复 */
    if ((s_light_restore_at != 0U) && App_TimeReached(s_light_restore_at))
    {
        s_light_restore_at = 0U;
        if (s_state != LOCK_STATE_LOCKED)
        {
            FPC_SetLight((s_fp_fail >= FGP_MAX_VERIFY) ? FPC_LIGHT_RED_CONST
                                                       : FPC_LIGHT_BLUE_BREATH);
        }
    }

    /* 4. 电机排队动作 */
    if (s_open_requested && !StepperMotor_IsRunning())
    {
        s_open_requested = false;
        door_open();
    }
    if (s_close_requested && !StepperMotor_IsRunning())
    {
        s_close_requested = false;
        door_close();
    }

    /* 5. 开门后保持扭矩，直到关门 */
    if (s_door_open && !s_close_requested && !s_door_hold && !StepperMotor_IsRunning())
    {
        StepperMotor_HoldPosition();
        s_door_hold = true;
        HAL_GPIO_WritePin(GPIOA, Led2_Pin, GPIO_PIN_RESET);
    }

    /* 6. 开门自动关门 / 自动返回主界面 */
    if (s_state == LOCK_STATE_UNLOCKED)
    {
        if ((s_door_close_at != 0U) && App_TimeReached(s_door_close_at))
        {
            s_door_close_at = 0U;
            door_close();
        }

        if ((s_auto_return_at != 0U) && App_TimeReached(s_auto_return_at))
        {
            if ((s_door_close_at == 0U) && !s_close_requested && !StepperMotor_IsRunning())
            {
                s_auto_return_at = 0U;
                enter_main();
            }
            else
            {
                /* 等门完全关好再返回，避免界面和电机状态不一致 */
                s_auto_return_at = now + 200U;
            }
        }
    }

    /* 7. 提示界面自动返回 */
    if ((s_state == LOCK_STATE_MESSAGE) && App_TimeReached(s_message_deadline))
    {
        enter_after_message();
    }

    /* 8. 锁定倒计时（非阻塞） */
    if (s_state == LOCK_STATE_LOCKED)
    {
        if (App_TimeReached(s_lockout_until))
        {
            s_lockout_tick = 0U;
            enter_main();
        }
        else if (App_TimeReached(s_lockout_tick + 1000U))
        {
            uint32_t remaining;

            s_lockout_tick = now;
            remaining = (uint32_t)(((s_lockout_until - now) + 999U) / 1000U);
            UI_ShowLocked(remaining);
        }
    }

    /* 9. 主界面无操作熄屏 */
    if ((s_state == LOCK_STATE_MAIN) && !s_sleeping &&
        App_TimeReached(s_last_activity + SLEEPING_TIME_MS))
    {
        s_sleeping = true;
        LCD_SetBacklight(0U);
    }
}
