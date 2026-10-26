#include "UserInterface.h"
char real_pwd[PWD_MAX_LEN+1];
uint8_t is_opening=0;
uint16_t wakeup_time=0; //初始化休眠计时
uint8_t sleeping_flag=0; //睡眠标志位

extern uint8_t clear_fgp_flag;
extern uint8_t stepper_lock;
extern TIM_HandleTypeDef htim2;
extern uint8_t led1_control_flag;
extern uint8_t led2_control_flag;
extern uint32_t led1_end_time;
extern IWDG_HandleTypeDef hiwdg;
extern uint32_t led2_end_time;

//字符串常量
const uint8_t chinese_indexs1[]={0,1,2,3};//欢迎来到
const uint8_t chinese_indexs2[]={4,5,6};//榕华二
const uint8_t chinese_indexs3[]={7,8,9}; //已上锁
const uint8_t chinese_indexs4[]={10,11,12}; //状态:
const uint8_t chinese_indexs5[]={13,14,15,16,17}; //请输入密码
const uint8_t chinese_indexs6[]={16,17}; //密码
const uint8_t chinese_indexs7[]={39,40,41,42}; //密码正确
const uint8_t chinese_indexs8[]={0,1,27,29}; //欢迎回宿
const uint8_t chinese_indexs9[]={20,21,22,23,24,25,26};//自动回到主界面
const uint8_t chinese_indexs10[]={13,14,15,31,16,17}; //请输入新密码
const uint8_t chinese_indexs11[]={32,33,34,35}; //修改成功
const uint8_t chinese_indexs12[]={36,37,38}; //已锁机

MenuItem menu_items[] = {
    {"Reset password", Show_input_new_pwd_interface},
    {"Register FP", Show_regist_FPG_interface},
	{"Clear all FP",Show_Clear_all_FP},
};

INTERFACE_STATUS now_interface=NO_INTERFACE; //默认无状态
AWESOME_LOCK_STATUS now_status=NORMAL_STATUS; //正常运行状态

char pwd[PWD_MAX_LEN+1]; //当前输入的密码数组，默认清成0，多一位不用额外加\0
char pwd_inputed_index=0; //存写入密码的序号
void None_handle(void){} //占位回调
uint8_t Verify_fail=0; //校验失败的次数
uint8_t FP_Verify_fail=0; //指纹校验失败的次数


//锁门回调
void Service_lock_handle(void) { 
	Service_close_door();
}

//回主界面回调
void Service_return_main_handle(void) {
	if(now_interface!=MAIN_INTERFACE)
	{
		Show_main_interface();
	}
}

void Show_main_interface(void) //主界面
{
	Service_clear_inputed_pwd();
	now_interface = MAIN_INTERFACE; //更新状态机
	LCD_Clear(SILVER);
	LCD_ShowChineseString16x16(10,10,chinese_indexs1,sizeof(chinese_indexs1),BLACK);
	LCD_ShowChineseString16x16(42,30,chinese_indexs2,sizeof(chinese_indexs2),BLACK);
	LCD_ShowString(90,30,"1115",Font_8x16,BLACK);
	
    // 2. 状态显示
	
	LCD_DrawRectangle(0, 50, LCD_WIDTH-1, 50, GRAY);
	
	LCD_ShowChineseString16x16(16,60,chinese_indexs4,sizeof(chinese_indexs4),BLACK); //状态
	LCD_ShowChineseString16x16(60,60,chinese_indexs3,sizeof(chinese_indexs3),RED); //已上锁
	
	// 画宿舍楼
	// 楼身（多层宿舍）
	LCD_DrawRectangle(30, 100, 90, 159, BLUE);
	LCD_Fill(31, 101, 89, 158, BLUE);

	// 楼层分隔线
	LCD_DrawRectangle(30, 120, 90, 120, WHITE);
	LCD_DrawRectangle(30, 140, 90, 140, WHITE);

	// 多个窗户
	// 一楼窗户
	LCD_DrawRectangle(40, 145, 50, 155, YELLOW);
	LCD_Fill(41, 146, 49, 154, YELLOW);
	LCD_DrawRectangle(70, 145, 80, 155, YELLOW);
	LCD_Fill(71, 146, 79, 154, YELLOW);

	// 二楼窗户
	LCD_DrawRectangle(40, 125, 50, 135, YELLOW);
	LCD_Fill(41, 126, 49, 134, YELLOW);
	LCD_DrawRectangle(70, 125, 80, 135, YELLOW);
	LCD_Fill(71, 126, 79, 134, YELLOW);

	// 三楼窗户
	LCD_DrawRectangle(40, 105, 50, 115, YELLOW);
	LCD_Fill(41, 106, 49, 114, YELLOW);
	LCD_DrawRectangle(70, 105, 80, 115, YELLOW);
	LCD_Fill(71, 106, 79, 114, YELLOW);

	// 宿舍门（大门）
	LCD_DrawRectangle(55, 145, 65, 159, BROWN);
	LCD_Fill(56, 146, 64, 158, BROWN);
}

void Show_input_interface(void) //开门密码验证界面
{
	now_interface=INPUT_INTERFACE;//更新状态机
	Service_clear_inputed_pwd(); //清空密码变量
	
    LCD_Clear(SILVER);
    
    // 顶部标题
    LCD_ShowChineseString16x16(10, 10, chinese_indexs5, sizeof(chinese_indexs5), BLACK); // 请输入密码
    
    // 分隔线
    LCD_DrawRectangle(0, 35, LCD_WIDTH-1, 35, GRAY);
    
    // 密码显示区域
    LCD_ShowChineseString16x16(10, 45, chinese_indexs6, sizeof(chinese_indexs6), BLACK); // 密码
    LCD_DrawRectangle(50, 43, 114, 63, BLACK);
    LCD_Fill(51, 44, 112, 62, WHITE);
}
void Show_success_interface(void) //密码正确开门界面
{
	now_interface = SUCCESS_INTERFACE; //更新状态机
	
    LCD_Clear(SILVER);
    
    // 顶部标题 - 密码正确
    LCD_ShowChineseString16x16(32, 15, chinese_indexs7, sizeof(chinese_indexs7), GREEN);
    
    // 分隔线
    LCD_DrawRectangle(10, 40, 118, 40, GRAY);
    
    // 欢迎回宿 - 居中显示
    LCD_ShowChineseString16x16(32, 50, chinese_indexs8, sizeof(chinese_indexs8), BLACK);
    
	LCD_ShowBedImage(36,75);
    // 底部提示 - 自动返回
    LCD_ShowChineseString16x16(10, 130, chinese_indexs9, sizeof(chinese_indexs9), DARK_GRAY);
}

void Show_admin_list_interface(void) //管理员管理界面
{
	now_interface=ADMIN_INTERFACE;
	Menu_Init(menu_items,sizeof(menu_items)/sizeof(MenuItem));
	Menu_Draw();
}
void Show_regist_FPG_interface(uint8_t index) //设置新指纹界面
{
	now_interface=ADMIN_REGIST_FP;
	FGP_Status_Code status_code;
	LCD_Clear(SILVER);
	
	LCD_ShowString(10,10,"Please put your finger down",Font_8x16,BLACK);
	status_code=FPC_register();
	LCD_Clear(SILVER);

	if(status_code==FGP_STATUS_OK)
	{
		LCD_ShowString(10,10,"OK",Font_8x16,BLACK);
		Show_successful_changed_password_interface();
		return;
	}
	else if(status_code==FGP_STATUS_FINGER_EXISTS)
	{
		LCD_ShowString(10,10,"Fingerprint are already exists",Font_8x16,BLACK);
		MultiTimer_Start(0, None_handle, 2, Service_return_main_handle);
		return;
	}
	else if(status_code==FGP_STATUS_TIMEOUT)
	{
		LCD_ShowString(10,10,"Timeout",Font_8x16,BLACK);
		MultiTimer_Start(0, None_handle, 2, Service_return_main_handle);
		return;
	}
	else if(status_code<=0x07 && status_code >=0x04)
	{
		LCD_ShowString(10,10,"Bad FP",Font_8x16,BLACK);
		MultiTimer_Start(0, None_handle, 2, Service_return_main_handle);
		return;
	}
	else
	{
		char str[20];
		sprintf(str,"Error Code:0X%02X",status_code);
		LCD_ShowString(10,10,str,Font_8x16,BLACK);
		MultiTimer_Start(0, None_handle, 2, Service_return_main_handle);
		return;
	}
}
void Show_input_new_pwd_interface(uint8_t index) //管理员输入新密码界面
{
	now_interface=ADMIN_SET_PWD_INTERFACE; //更新状态机
	Service_clear_inputed_pwd(); //清空密码变量
	
    LCD_Clear(SILVER);
    
    // 顶部标题
    LCD_ShowChineseString16x16(10, 10, chinese_indexs10, sizeof(chinese_indexs10), BLACK); // 请输入密码
    
    // 分隔线
    LCD_DrawRectangle(0, 35, LCD_WIDTH-1, 35, GRAY);
    
    // 密码显示区域
    LCD_ShowChineseString16x16(10, 45, chinese_indexs6, sizeof(chinese_indexs6), BLACK); // 密码
    LCD_DrawRectangle(50, 43, 114, 63, BLACK);
    LCD_Fill(51, 44, 112, 62, WHITE);
	
}
void Show_Admin_check_interface(void) //管理员验证界面
{
	now_interface=ADMIN_CHECK_INTERFACE;//更新状态机
	Service_clear_inputed_pwd(); //清空密码变量
	
    LCD_Clear(SILVER);
    
    // 顶部标题
    LCD_ShowChineseString16x16(10, 10, chinese_indexs5, 3, BLACK); // 请输入密码
	LCD_ShowString(58,10,"Adminpwd",Font_8x16,BLACK);
    
    // 分隔线
    LCD_DrawRectangle(0, 35, LCD_WIDTH-1, 35, GRAY);
    
    // 密码显示区域
    LCD_ShowChineseString16x16(10, 45, chinese_indexs6, sizeof(chinese_indexs6), BLACK); // 密码
    LCD_DrawRectangle(50, 43, 114, 63, BLACK);
    LCD_Fill(51, 44, 112, 62, WHITE);
}
void Show_successful_changed_password_interface(void) //管理员成功修改密码界面
{
	now_interface=SUCCESSFUL_CHANGE_PWD_INTERFACE;//更新状态机
	Service_clear_inputed_pwd(); //清空密码变量
	
	LCD_Clear(SILVER);
	
    // 顶部标题 - 修改成功
    LCD_ShowChineseString16x16(32, 15, chinese_indexs11, sizeof(chinese_indexs11), GREEN);
    
    // 分隔线
    LCD_DrawRectangle(10, 40, 118, 40, GRAY);
    
	LCD_ShowBedImage(36,60);
    // 底部提示 - 自动返回
    LCD_ShowChineseString16x16(10, 130, chinese_indexs9, sizeof(chinese_indexs9), DARK_GRAY);
	
	MultiTimer_Start(0, None_handle, 1, Service_return_main_handle);
	
}
void Show_updated_pwd(uint8_t nums) //只要传入密码的位数就可以
{
	LCD_Fill(51, 44, 112, 62, WHITE); //填充白色区域
	uint8_t showed_x=55;
	if(nums<=7)
	{
		for(uint8_t x=0;x<nums;x++)
		{
			LCD_ShowString(showed_x, 45, "*", Font_8x16, BLACK);
			showed_x+=8;
		}
	}
	else
	{
		for(uint8_t x=0;x<7;x++)
		{
			LCD_ShowString(showed_x, 45, "*", Font_8x16, BLACK);
			showed_x+=8;
		}
	}
}
void Show_locked_interface(void)
{
	now_interface=LOCKED_INTERFACE;
    LCD_Clear(SILVER);
    
    // 顶部标题 - 已锁机
    LCD_ShowChineseString16x16(40, 15, chinese_indexs12, sizeof(chinese_indexs12), RED);
    
    // 分隔线
    LCD_DrawRectangle(10, 40, 118, 40, GRAY);
	LCD_ShowBedImage(36,75);
}
bool Service_add_pwd(char num) //注意传入的是字符类型不是数字
{
	if(pwd_inputed_index<=PWD_MAX_LEN-1)
	{
		pwd[pwd_inputed_index]=num; //密码写入
		pwd_inputed_index++; //移位
		return true; //密码成功写入到pwd数组中
	}
	else
	{
		return false; //密码写入失败,数组已经满了
	}
}
char* Service_get_real_pwd(void)
{
    // 先检查是否已写入密码
    if(FLASH_EEPROM_IsPasswordWritten() == 0)
    {
        // 未写入过密码，返回默认密码
        return DEFULT_PWD;
    }
    else
    {
        // 已写入过密码，尝试读取
        if(FLASH_EEPROM_ReadPassword(real_pwd) == HAL_OK)
        {
            // 额外验证读取的密码是否有效（不是空字符串或无效数据）
            if(strlen(real_pwd) > 0 && real_pwd[0] != 0xFF)
            {
                return real_pwd;
            }
            else
            {
                // 读取的数据无效，返回默认密码
                return DEFULT_PWD;
            }
        }
        else
        {
            // 读取失败，返回NULL表示错误
            return NULL;
        }
    }
}
char* Service_get_real_admin_pwd(void) //获取系统中存储的正确密码,是一个字符串
{
	return ADMIN_PWD;
}
/*
直接用全局的pwd变量和Service_get_real_pwd()的返回值比较
密码和real_pwd相等返回true否则返回false
*/
bool Service_cmp_pwd(void)
{
    char *geted_real_password = Service_get_real_pwd();
    uint8_t x = 0;
    
    // 同时检查两个字符串
    while(geted_real_password[x] != '\0' && pwd[x] != '\0')
    {
        if(geted_real_password[x] != pwd[x])
            return false;
        x++;
    }
    
    // 必须同时结束
    return (geted_real_password[x] == '\0' && pwd[x] == '\0');
}
bool Service_cmp_admin_pwd(void)
{
    char *geted_real_password = Service_get_real_admin_pwd();
    uint8_t x = 0;
    
    // 同时检查两个字符串
    while(geted_real_password[x] != '\0' && pwd[x] != '\0')
    {
        if(geted_real_password[x] != pwd[x])
            return false;
        x++;
    }
    
    // 必须同时结束
    return (geted_real_password[x] == '\0' && pwd[x] == '\0');
}
//用于清空pwd
void Service_clear_inputed_pwd(void)
{
	uint8_t x=0;
	for(x=0;x<PWD_MAX_LEN-1;x++)
	{
		pwd[x]=0; //整个pwd数组直接置0
	}
	pwd_inputed_index=0; //pwd长度也置0
}
//退格
//返回值true退格成功(密码长度大于1)，否则false
bool Service_pop_inputed_pwd(void)
{

	if(pwd_inputed_index>0) //已输入密码有长度
	{
		pwd[pwd_inputed_index-1]='\0'; //移位置0
		pwd_inputed_index--;
		return true;
	}
	else
	{
		return false;
	}
}
//触摸键盘回调
void Service_KeyboardHandler(void)
{
	if(now_status==LOCKED_STATUS){return;} //锁机模式不理会
	char inputed_char;
	inputed_char=Keyboard_GetChar(Keyboard_get()); //输入的字符
	//uart_printf("Pressed_key:%c\r\n",inputed_char);
	if(inputed_char <= '9' && inputed_char >= '0') //如果输入的是数字
	{
		if(now_interface==INPUT_INTERFACE) //如果在输入密码界面
		{
			Service_add_pwd(inputed_char); //为pwd传入字符
			Show_updated_pwd(pwd_inputed_index); //显示*号,传入正好是目前写入pwd的索引
		}
		else if(now_interface==MAIN_INTERFACE) //如果在主界面
		{
			Show_input_interface(); //切换到输入密码界面
			Service_add_pwd(inputed_char); //为pwd传入字符
			Show_updated_pwd(pwd_inputed_index); //显示*号,传入正好是目前写入pwd的索引
		}
		else if(now_interface==ADMIN_CHECK_INTERFACE) //如果在管理员验证界面
		{
			Service_add_pwd(inputed_char); //为pwd传入字符
			Show_updated_pwd(pwd_inputed_index); //显示*号,传入正好是目前写入pwd的索引
			//uart_printf(pwd);
		}
		else if(now_interface==ADMIN_SET_PWD_INTERFACE) //如果在管理员密码设置界面
		{
			Service_add_pwd(inputed_char); //为pwd传入字符
			Show_updated_pwd(pwd_inputed_index); //显示*号,传入正好是目前写入pwd的索引
			//uart_printf(pwd);
		}
	}
	if(inputed_char=='#') //如果输入的是#
	{
		if(now_interface==INPUT_INTERFACE) //如果在输入密码界面
		{
			if(Service_cmp_pwd()==true)//校验密码
			{ 
				Allow_use_FP(); //允许使用指纹
				Verify_fail=0; //重置失败次数
				Show_success_interface();
				Service_open_door(); //开门
				
				if(OPEN_DOOR_TIME<OPEN_DOOR_RETURN_MAIN_TIME)
				{
					MultiTimer_Start(OPEN_DOOR_TIME, Service_lock_handle, OPEN_DOOR_RETURN_MAIN_TIME, Service_return_main_handle);
				}
				else
				{
					MultiTimer_Start(OPEN_DOOR_RETURN_MAIN_TIME, Service_return_main_handle, OPEN_DOOR_TIME, Service_lock_handle);
				}
			}
			else
			{
				//校验错误
				//uart_printf("Fail \r\nnow:%s\r\nreal%s\r\n",pwd,Service_get_real_pwd());
				if(Verify_fail<3)
				{
					Service_clear_inputed_pwd();
					pwd_inputed_index=0;
					Show_updated_pwd(pwd_inputed_index); //清空密码
				}
				Verify_fail_publish();
				
			}
		}
		else if(now_interface==ADMIN_CHECK_INTERFACE) //如果在管理员校验界面
		{
			if(Service_cmp_admin_pwd()==true)//校验密码
			{ 
				//校验正确
				Verify_fail=0; //重置失败次数
				Show_admin_list_interface();
				
			}
			else
			{
				//校验错误
				if(Verify_fail<3)
				{
					Service_clear_inputed_pwd();
					pwd_inputed_index=0;
					Show_updated_pwd(pwd_inputed_index); //清空密码
				}//uart_printf("Fail_admin \r\nnow:%s\r\n",pwd);
				
				Verify_fail_publish();
			}
		}
		else if(now_interface==ADMIN_SET_PWD_INTERFACE) //如果在管理员设置密码界面
		{
			//uart_printf("%d\r\n",pwd_inputed_index);
			if(pwd_inputed_index<PWD_MIN_LEN)//输入密码过短
			{
				char warning_min_len[12];
				sprintf(warning_min_len,"At latest %d",PWD_MIN_LEN);
				LCD_ShowString(8,140,warning_min_len,Font_8x16,RED);
			}
			else
			{
				Service_change_pwd(pwd); //修改密码
				Show_successful_changed_password_interface(); //密码改好了提示界面
			}
		}
	}
	if(inputed_char=='A') //如果输入的是A
	{
		if(now_interface==MAIN_INTERFACE) //主界面
		{
			Show_Admin_check_interface();
		}
		else if(now_interface==ADMIN_INTERFACE) //如果在管理员设置界面
		{
			Menu_Up();
			Menu_Draw();
		}
	}
	if(inputed_char=='B')
	{
		if(now_interface==ADMIN_INTERFACE) //如果在管理员设置界面
		{
			Menu_Down();
			Menu_Draw();
		}
	}
	if(inputed_char=='C')
	{
		if(now_interface==ADMIN_INTERFACE) //如果在管理员设置界面
		{
			Menu_Select();
		}
	}
	if(inputed_char=='D')
	{
		if(now_interface != MAIN_INTERFACE)
		{
			
			Show_main_interface();
		}
	}
	if(inputed_char=='*')
	{
		if(now_interface==INPUT_INTERFACE | now_interface==ADMIN_CHECK_INTERFACE | now_interface==ADMIN_SET_PWD_INTERFACE) //如果在输入密码界面
		{
			Service_pop_inputed_pwd(); //回退
			Show_updated_pwd(pwd_inputed_index); //刷新密码索引
			//uart_printf("Poped \r\nnow:%s\r\n",pwd);
		}
	}
}
bool Service_change_pwd(char *new_pwd) //注意这里传进来的是字符串
{
	if(FLASH_EEPROM_WritePassword(new_pwd)==HAL_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}
//开门
void Service_open_door(void)
{
	if(is_opening==0)
	{
		#ifdef MOTOR_REVERSE
			StepperMotor_RotateAngle(MOTOR_CCW,OPEN_DOOR_ANGLE,OPEN_DOOR_SPEED);
		#endif
		#ifndef MOTOR_REVERSE
			StepperMotor_RotateAngle(MOTOR_CW,OPEN_DOOR_ANGLE,OPEN_DOOR_SPEED);
		#endif
		is_opening=1;
	}
}
//关门
void Service_close_door(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,1);//关闭led2
	if(is_opening==1){
		stepper_lock=0;
		#ifdef MOTOR_REVERSE
			StepperMotor_RotateAngle(MOTOR_CW,OPEN_DOOR_ANGLE,OPEN_DOOR_SPEED);
		#endif
		#ifndef MOTOR_REVERSE
			StepperMotor_RotateAngle(MOTOR_CCW,OPEN_DOOR_ANGLE,OPEN_DOOR_SPEED);
		#endif
		is_opening=0;
	}
}
void printf_real_pwd(void)
{
	//uart_printf("real_pwd:%s\r\n",Service_get_real_pwd());
}
void Verify_fail_publish(void)
{
	//校验失败统一回调
	Verify_fail++; //校验失败次数加一
	if(Verify_fail>=FAIL_LOCK_COUNT) //校验失败次数大于三
	{
		now_status=LOCKED_STATUS; //切换锁机模式
		Show_locked_interface();
		for(volatile uint32_t x=0;x<FAIL_LOCK_TIME;x++) //粗略延时
		{
			HAL_IWDG_Refresh(&hiwdg);//喂狗
			for(volatile uint32_t i = 0; i < 16800000; i++) {
				__NOP(); // 空操作，防止编译器优化
			}
			//uart_printf("Locked:%d\r\n",FAIL_LOCK_TIME-x);
		}
		Verify_fail=0; //重置失败次数
		wakeup_time=0; //重置休眠定时器
		now_status=NORMAL_STATUS; //切换正常模式
		Show_main_interface();
	}
}
void FP_Verify_fail_publish(void) //指纹校验失败回调
{
	FP_Verify_fail++; //增加失败次数
	if(FP_Verify_fail>FGP_MAX_VERIFY)
	{
		FGP_Constant_light(FGP_LIGHT_RED); //设置灯变成红色常亮
	}
	
}
void Show_Clear_all_FP(uint8_t index)
{
	clear_fgp_flag=1;
	Show_successful_changed_password_interface();
}
void Allow_use_FP(void)
{
	FP_Verify_fail=0;
	FGP_Breath_light(FGP_LIGHT_BLUE,FGP_LIGHT_BLUE,0,2); //开启蓝灯
}
void Service_FP_pressed(void)
{
	if((now_interface==MAIN_INTERFACE || now_interface==INPUT_INTERFACE) && FP_Verify_fail <=FGP_MAX_VERIFY)
	{
		FGP_CHECK_RESPONSE response=FPC_check();
		//这里处理指纹校验
		if(response.is_successed==true) //校验通过
		{
			//校验正确
			Verify_fail=0; //重置失败次数
			FP_Verify_fail=0;
			Show_success_interface();
			Service_open_door(); //开门
			if(OPEN_DOOR_TIME<OPEN_DOOR_RETURN_MAIN_TIME)
			{
				MultiTimer_Start(OPEN_DOOR_TIME, Service_lock_handle, OPEN_DOOR_RETURN_MAIN_TIME, Service_return_main_handle);
			}
			else
			{
				MultiTimer_Start(OPEN_DOOR_RETURN_MAIN_TIME, Service_return_main_handle, OPEN_DOOR_TIME, Service_lock_handle);
			}
		}
		else //校验失败
		{
			FP_Verify_fail_publish();
			//uart_printf("fail");
		}
	}
}

void Service_press_indoor_button(void)
{
	Service_open_door();
	MultiTimer_Start(0,None_handle , OPEN_DOOR_TIME, Service_close_door);

	LCD_SetBacklight(0); //开启显示器背光
}
//进入休眠
void Service_sleep(void)
{
	if(sleeping_flag==0)
	{
		sleeping_flag=1; //睡眠标志位置1
		HAL_TIM_Base_Stop_IT(&htim2); //停止计时
		
		LCD_SetBacklight(0); //关闭显示器背光
	}
}
//唤醒
void Service_wakeup(void)
{
	wakeup_time=0; //无论如何，清零睡眠变量
	if(sleeping_flag==1)
	{
		sleeping_flag=0; //睡眠标志位置0
		HAL_TIM_Base_Start_IT(&htim2); //重新开始计时
		
		LCD_SetBacklight(1); //开启显示器背光
	}
	
}
void wakeup_tim_handler(void)
{
	if(now_status!=LOCKED_STATUS){
		if(wakeup_time==SLEEPING_TIME) //计数到了，进入休眠
		{
			Service_sleep();
			if(now_interface==ADMIN_INTERFACE)
			{
				Show_main_interface(); //如果在管理界面就返回主界面
			}
		}
		wakeup_time++;
	}
}
void close_led1(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,1);
}
void close_led2(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,1);
}
void Service_light_up_led1(uint16_t time) //time单位秒，led亮的时间
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,0);
	MultiTimer_Start(0,None_handle,time,close_led1);
}
void Service_light_up_led2(uint16_t time)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,0);
	MultiTimer_Start(0,None_handle,time,close_led2);
}