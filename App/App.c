#include "App.h"
uint8_t clear_fgp_flag=0;
uint8_t FGP_Check_Flag=0;
uint8_t stepper_lock=0;
uint8_t have_keep_pos=0;
extern uint8_t is_opening;
extern UART_HandleTypeDef huart4;
extern uint16_t wakeup_time; //初始化休眠计时
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;
extern IWDG_HandleTypeDef hiwdg;
extern uint8_t sleeping_flag;
void App_Init(void)
{
	//HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	//初始化一堆东西先
	//Print_Boot_Info();
	//uart_printf("Booting\r\n");
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,1);//关闭led1
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,1);//关闭led2
	StepperMotor_Init(&htim6);

	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim4);
	
	LCD_Init();
	Show_main_interface();
	
	//StepperMotor_RotateAngle(MOTOR_CW,120,OPEN_DOOR_SPEED);
	FGP_Breath_light(FGP_LIGHT_BLUE,FGP_LIGHT_BLUE,0,2);
}
void App_Running(void)
{
	if(clear_fgp_flag==1)
	{
		clear_fgp_flag=0;
		FPC_clear();
	}
	else if(FGP_Check_Flag==1)
	{
		FGP_Check_Flag=0;
		Service_FP_pressed();
	}
	
	if(is_opening==1 && StepperMotor_IsRunning()==0 && have_keep_pos==0){
		StepperMotor_HoldPosition();
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,0);//开启led2
		have_keep_pos=1;
	}
	else if(is_opening==0 && StepperMotor_IsRunning()==0 && have_keep_pos==1){
		StepperMotor_Stop();
		
		have_keep_pos=0;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	
    StepperMotor_TIM6_Callback(htim); //这里面有判断是不是tim6，放心写
	if(htim->Instance==TIM4){
		HAL_IWDG_Refresh(&hiwdg);
	}
	else if(htim->Instance==TIM2)
	{
		//uart_printf("%d",wakeup_time);
		wakeup_tim_handler(); //睡眠定时器回调
		
		//uart_printf("%d",wakeup_time);
	}
}
// 外部中断处理
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_5)
    {
		Service_wakeup(); //唤醒
		Service_KeyboardHandler(); //键盘按下中断处理
    }
	else if(GPIO_Pin == GPIO_PIN_8 && stepper_lock!=1&& StepperMotor_IsRunning()==0)
	{
		Service_wakeup(); //唤醒
		FGP_Check_Flag=1; //处理指纹按下标志位
	}
	//处理霍尔传感器关门
//	else if(GPIO_Pin == GPIO_PIN_7 && is_opening==1&& StepperMotor_IsRunning()==0)
//	{
//		stepper_lock=1;
//		Service_close_door();
//	}
	else if(GPIO_Pin ==GPIO_PIN_0) //人体感应模块感应到
	{
		#ifdef BODY_WAKEUP
			Service_wakeup(); //唤醒
		#endif
	}
}
