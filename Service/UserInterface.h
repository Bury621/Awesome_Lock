#ifndef __USERINTERACE_H
#define __USERINTERACE_H

#include "main.h"
#include "lcd.h"
#include "stdbool.h"
#include "Debug.h"
#include "Keyboard.h"
#include "stepper_motor.h"
#include "multi_timer.h"
#include "flash_eeprom.h"
#include "Fingerprint.h"
#include <string.h>
#include <stdio.h>
#include "lcd_menu.h"

//枚举定义
typedef enum
{
	NO_INTERFACE=0, //无状态
	MAIN_INTERFACE, //主界面
	INPUT_INTERFACE, //输入界面
	SUCCESS_INTERFACE, //成功开锁界面
	ADMIN_CHECK_INTERFACE, //管理员验证界面
	ADMIN_INTERFACE, //管理员管理界面
	ADMIN_SET_PWD_INTERFACE, //设置新密码界面
	SUCCESSFUL_CHANGE_PWD_INTERFACE, //成功修改密码界面
	LOCKED_INTERFACE, //锁机界面
	ADMIN_REGIST_FP, //注册新指纹界面
	
}INTERFACE_STATUS; //定义界面状态机

typedef enum
{
	NORMAL_STATUS=0, //正常运行状态
	LOCKED_STATUS, //锁机状态
	SLEEPING_STATUS,//休眠状态
}AWESOME_LOCK_STATUS;
extern INTERFACE_STATUS now_interface;
extern uint8_t inputed_len; //已经输入的密码长度
//函数定义
void Show_main_interface(void);
void Show_input_interface(void);
void Show_success_interface(void);
void Show_Admin_interface(void);
void Show_updated_pwd(uint8_t nums);
bool Service_add_pwd(char num);
void Service_clear_inputed_pwd(void);
bool Service_cmp_pwd(void);
void Service_KeyboardHandler(void);
void Service_open_door(void);
void Service_close_door(void);
void Show_success_admin_interface(void);
void Verify_fail_publish(void);
bool Service_change_pwd(char *new_pwd);
void Show_successful_changed_password_interface(void);
void Show_locked_interface(void);
void printf_real_pwd(void);
void Show_input_new_pwd_interface(uint8_t index);
void Show_regist_FPG_interface(uint8_t index);
void Show_Clear_all_FP(uint8_t index);
void None_handle(void);
void Service_FP_pressed(void);
void FP_Verify_fail_publish(void);
void Allow_use_FP(void);
void Service_press_indoor_button(void);
void wakeup_tim_handler(void);
void Service_wakeup(void);
void Service_light_up_led1(uint16_t time);
void Service_light_up_led2(uint16_t time);
#endif