#include "lock_app.h"
#include "app_util.h"
#include "ui_views.h"
#include "Keyboard.h"
#include "stepper_motor.h"
#include "flash_eeprom.h"
#include "Fingerprint.h"
#include "Debug.h"
#include <stdio.h>
#include <string.h>

/* 鐢垫満鏂瑰悜锛氭寜瀹為檯瑁呴厤鏂瑰悜閫夋嫨 */
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
static uint32_t s_motor_deadline;
/*
 * 鐢垫満杞姩鏈熼棿涓存椂鍏虫帀澶栭儴涓柇婧愩€? * 瀹炴満鎶撳埌鐨勫崱姝?閲嶅惎鍙戠敓鍦ㄧ數鏈鸿浆鍔ㄧ獥鍙ｅ唴锛屼笖浼撮殢 EXTI9_5/UART5 娲诲姩锛? * 寮€闂?鍏抽棬杩?1 绉掑唴涓嶉渶瑕佹寜閿€佹寚绾硅Е鎽稿拰闂ㄧ锛屽厛鍏?NVIC锛? * 閬垮厤浠讳綍杈规部鎶栧姩/涓插彛閿欒鎶婁富寰幆楗挎銆傜數鏈哄仠绋冲悗绔嬪嵆鎭㈠銆? */
static uint32_t s_light_restore_at;

static bool     s_fp_retry_armed;
static uint32_t s_fp_retry_at;

static uint8_t   s_menu_index;
static LockState s_message_return;
static uint32_t  s_message_deadline;

/* -------------------------------------------------------------------------- */
/* 鍓嶇疆澹版槑                                                                    */
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
/* 瀵嗙爜缂撳啿                                                                    */
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
/* 鐣岄潰鍒囨崲                                                                    */
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
    s_last_activity = HAL_GetTick();
    UI_ShowInput(mode, 0U);
}

static void enter_admin_menu(uint8_t index)
{
    pwd_clear();
    s_menu_index = index;
    s_state = LOCK_STATE_ADMIN_MENU;
    s_last_activity = HAL_GetTick();
    UI_ShowMenu(index);
}

static void show_message(const char *line1, const char *line2, uint16_t color,
                         LockState return_state, uint32_t duration_ms)
{
    s_state = LOCK_STATE_MESSAGE;
    s_message_return = return_state;
    s_message_deadline = HAL_GetTick() + duration_ms;
    s_last_activity = HAL_GetTick();
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

/* 鍝簺鐣岄潰鍏佽 5 绉掓棤鎿嶄綔鍚庣唲灞?*/
static bool state_allows_sleep(void)
{
    switch (s_state)
    {
        case LOCK_STATE_MAIN:
        case LOCK_STATE_INPUT:
        case LOCK_STATE_ADMIN_CHECK:
        case LOCK_STATE_ADMIN_MENU:
        case LOCK_STATE_ADMIN_NEW_PWD:
            return true;

        default:
            /* UNLOCKED / LOCKED / 鎸囩汗娉ㄥ唽绛夊緟涓笉鐔勫睆 */
            return false;
    }
}

/* -------------------------------------------------------------------------- */
/* 闂?/ 鐢垫満                                                                   */
/* -------------------------------------------------------------------------- */
/* 璁板綍涓€娆″紑鍏抽棬鐨勫悎鐞嗚秴鏃舵椂闂达紝鐢ㄤ簬鍙戠幇瀹氭椂鍣?涓柇寮傚父 */
static void door_move_started(void)
{
    uint32_t steps = StepperMotor_AngleToSteps((float)OPEN_DOOR_ANGLE);
    uint32_t move_ms = (steps * (uint32_t)OPEN_DOOR_STEP_US) / 1000U;

    s_motor_deadline = HAL_GetTick() + (move_ms * 2U) + 1000U;
}

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
    door_move_started();
    uart_printf("MOTOR OPEN t=%lu\r\n", (unsigned long)HAL_GetTick());
    HAL_GPIO_WritePin(GPIOA, Led2_Pin, GPIO_PIN_RESET); /* 寮€闂ㄦ寚绀虹伅浜?*/
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

    HAL_GPIO_WritePin(GPIOA, Led2_Pin, GPIO_PIN_SET); /* 寮€闂ㄦ寚绀虹伅鐏?*/
    StepperMotor_RotateAngle(DOOR_CLOSE_DIR, (float)OPEN_DOOR_ANGLE, OPEN_DOOR_STEP_US);
    door_move_started();
    uart_printf("MOTOR CLOSE t=%lu\r\n", (unsigned long)HAL_GetTick());
}

/* -------------------------------------------------------------------------- */
/* 瑙ｉ攣 / 閿佸畾                                                                 */
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
/* 鎸囩汗                                                                        */
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
        /* 鎸囩汗澶辫触娆℃暟杩囧锛氭殏鏃剁鐢ㄦ寚绾癸紝鏀圭敤瀵嗙爜 */
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
         * 鍒氬彂杩囩伅鍏夋寚浠ゆ椂 FPC 浼氭嫆缁濈珛鍗冲惎鍔紙閬垮厤 ACK 涓插寘锛夈€?         * 鐭殏寤惰繜鍚庤嚜鍔ㄩ噸璇曚竴娆★紝涓嶄涪鐢ㄦ埛鐨勮Е鎽搞€?         */
        s_fp_retry_armed = true;
        s_fp_retry_at = now + 200U;
    }
    else
    {
        /* 鍙湁鐪熸鍙戣捣浜嗘寚绾归獙璇侊紝鎵嶇畻涓€娆♀€滅敤鎴锋搷浣溾€?*/
        s_last_activity = now;
    }
}

static void on_fp_event(FPC_Op op, FPC_Result result, FGP_Status_Code status)
{
    uint32_t now = HAL_GetTick();

    uart_printf("FP result op=%u res=%u st=%02X\r\n",
                (unsigned)op, (unsigned)result, (unsigned)status);

    if (op == FPC_OP_VERIFY)
    {
        /* 鐢ㄦ埛鍙兘宸茬粡杩斿洖涓荤晫闈?杩涘叆绠＄悊鍛樼晫闈紝姝ゆ椂缁撴灉浣滃簾 */
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
            /* 瓒呮椂/閫氫俊澶辫触锛氫笉鎯╃綒鐢ㄦ埛锛屼繚鎸佸綋鍓嶇晫闈?*/
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
/* 鎸夐敭澶勭悊                                                                    */
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
             * 娉ㄥ唽杩囩▼涓笉鎺ュ彈杩斿洖閿細涓€鏃﹀彇娑堬紝妯″潡涔嬪悗杩熷埌鐨勫簲绛斿寘
             * 鍙兘琚笅涓€娆￠獙璇佽鍒わ紙鍗忚纭鍖呬笉甯︽寚浠ゅ彿锛夈€?             * 鏈€闀跨瓑寰?FGP_ENROLL_TIMEOUT_MS锛?0s锛夊悗浼氳嚜鍔ㄨ秴鏃惰繑鍥炪€?             */
            break;

        case LOCK_STATE_ADMIN_FP_BUSY:
            /* 鍚岀悊锛屾竻绌烘寚绾瑰簱鏈熼棿绛夊緟缁撴灉 */
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
/* 瀵瑰鎺ュ彛                                                                    */
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

    uart_printf("LOCK: flash init\r\n");
    FLASH_EEPROM_Init();
    uart_printf("LOCK: flash ok\r\n");

    UI_ShowMain();
    LCD_SetBacklight(1U);
    uart_printf("LOCK: ui ok\r\n");

    FPC_Init(on_fp_event);
    uart_printf("LOCK: fp init ok\r\n");
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
         * 鐔勫睆鐘舵€佷笅锛?         *   鎸夐敭 -> 鍙敜閱掞紝骞惰涓€娆?I2C 娓呮帀鎸夐敭妯″潡鐨勯攣瀛橈紱
         *   浜轰綋鎰熷簲 -> 鍙敜閱掞紱
         *   鎸囩汗瑙︽懜 -> 鍞ら啋鍚庣户缁蛋鎸囩汗楠岃瘉娴佺▼锛屼笉鐢ㄥ啀鎸夌浜屾銆?         */
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
            uart_printf("WAKE\r\n");

            if (event->type != APP_EVENT_FINGERPRINT)
            {
                return;
            }
            /* 鎸囩汗瑙︽懜锛氬敜閱掑悗缁х画鎵ц涓嬮潰鐨?fp_touch() */
        }
    }

    switch (event->type)
    {
        case APP_EVENT_KEYPAD:
            /*
             * 杩欓噷涓嶅埛鏂?s_last_activity锛?             * 鍙湁鐪熸璇诲埌鏈夋晥閿€硷紙LockApp_Poll 閲岋級鎵嶇畻鐢ㄦ埛鎿嶄綔锛?             * 闃叉閿洏 IRQ 鍙楀共鎵版椂涓€鐩存妸浼戠湢璁℃椂娓呴浂銆?             */
            if (!s_keypad_pending)
            {
                s_keypad_pending = true;
                s_keypad_due = now + KEYPAD_DEBOUNCE_MS;
            }
            break;

        case APP_EVENT_FINGERPRINT:
            /* 灞忓箷浜潃鏃舵寜鎸囩汗锛氶噸鏂拌鏃?5 绉?*/
            s_last_activity = now;
            fp_touch();
            break;

        case APP_EVENT_BODY:
            /* 浜洪潬杩戯細灞忓箷浜潃鏃堕噸鏂拌鏃?5 绉掞紱鐔勫睆鏃跺凡鍦ㄤ笂闈㈠敜閱?*/
            s_last_activity = now;
            break;

        case APP_EVENT_DOOR_SENSE:
        default:
            break;
    }
}

void LockApp_Poll(void)
{
    uint32_t now = HAL_GetTick();

    /* 1. 鎸夐敭鍘绘姈鍚庤鍙栵紙I2C 鍙湪涓诲惊鐜噷璁块棶锛?*/
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
            /* 璇诲け璐ユ椂鐭殏閲嶈瘯锛屼笉闃诲涓诲惊鐜?*/
            s_keypad_pending = true;
            s_keypad_due = now + 50U;
        }
    }

    /* 2. 鎸囩汗楠岃瘉寤惰繜閲嶈瘯锛堝垰鍙戣繃鐏厜鎸囦护鏃朵細鐭殏鎷掔粷鍚姩锛?*/
    if (s_fp_retry_armed && App_TimeReached(s_fp_retry_at))
    {
        s_fp_retry_armed = false;
        if (((s_state == LOCK_STATE_MAIN) || (s_state == LOCK_STATE_INPUT)) &&
            (s_fp_fail < FGP_MAX_VERIFY) && !FPC_IsBusy())
        {
            if (FPC_StartVerify())
            {
                s_last_activity = now;
            }
        }
    }

    /* 3. 鎸囩汗鐏仮澶?*/
    if ((s_light_restore_at != 0U) && App_TimeReached(s_light_restore_at))
    {
        s_light_restore_at = 0U;
        if (s_state != LOCK_STATE_LOCKED)
        {
            FPC_SetLight((s_fp_fail >= FGP_MAX_VERIFY) ? FPC_LIGHT_RED_CONST
                                                       : FPC_LIGHT_BLUE_BREATH);
        }
    }

    /* 4. 鐢垫満鎺掗槦鍔ㄤ綔 */
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

    /* 5. 鐢垫満杩愯瓒呮椂淇濇姢锛氭甯镐竴娆″紑鍏抽棬绾?1.2~1.5 绉掞紝瓒呰繃鍚堢悊涓婇檺寮哄埗鍋滄 */
    if (StepperMotor_IsRunning())
    {
        if ((s_motor_deadline != 0U) && App_TimeReached(s_motor_deadline))
        {
            uart_printf("MOTOR TIMEOUT t=%lu\r\n", (unsigned long)HAL_GetTick());
            StepperMotor_HoldPosition();
            s_door_hold = true;
            s_motor_deadline = 0U;
        }
    }
    else
    {
        s_motor_deadline = 0U;
    }

    /* 鐢垫満鍋滅ǔ鍚庢仮澶嶅閮ㄤ腑鏂?*/

    /* 6. 寮€闂ㄥ悗淇濇寔鎵煩锛岀洿鍒板叧闂?*/
    if (s_door_open && !s_close_requested && !s_door_hold && !StepperMotor_IsRunning())
    {
        StepperMotor_HoldPosition();
        s_door_hold = true;
        uart_printf("MOTOR HOLD t=%lu\r\n", (unsigned long)HAL_GetTick());
        HAL_GPIO_WritePin(GPIOA, Led2_Pin, GPIO_PIN_RESET);
    }

    /* 7. 寮€闂ㄨ嚜鍔ㄥ叧闂?/ 鑷姩杩斿洖涓荤晫闈?*/
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
                /* 绛夐棬瀹屽叏鍏冲ソ鍐嶈繑鍥烇紝閬垮厤鐣岄潰鍜岀數鏈虹姸鎬佷笉涓€鑷?*/
                s_auto_return_at = now + 200U;
            }
        }
    }

    /* 7. 鎻愮ず鐣岄潰鑷姩杩斿洖 */
    if ((s_state == LOCK_STATE_MESSAGE) && App_TimeReached(s_message_deadline))
    {
        enter_after_message();
    }

    /* 8. 閿佸畾鍊掕鏃讹紙闈為樆濉烇級 */
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

    /* 9. 鏃犳搷浣滅唲灞忥細涓荤晫闈?瀵嗙爜杈撳叆/绠＄悊鍛樼晫闈㈤兘閫傜敤 */
    if (!s_sleeping && state_allows_sleep() &&
        App_TimeReached(s_last_activity + SLEEPING_TIME_MS))
    {
        /* 浠庡瘑鐮?绠＄悊鍛樼晫闈㈢唲灞忔椂锛岄『鎵嬪洖鍒颁富鐣岄潰锛岄伩鍏嶅崐鎴緭鍏ョ暀鍦ㄥ睆骞曚笂 */
        if (s_state != LOCK_STATE_MAIN)
        {
            enter_main();
        }

        s_sleeping = true;
        LCD_SetBacklight(0U);
        uart_printf("SLEEP\r\n");
    }
}

