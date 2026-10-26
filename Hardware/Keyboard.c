#include "main.h"
#include "Keyboard.h"

//0852*741DCBA#963
uint16_t Keyboard_get(void)
{
	uint8_t rx_data[2]; //接收缓冲区
	HAL_StatusTypeDef status;
	status = HAL_I2C_Master_Receive(&hi2c1,KEYBOARD_ADDRESS<<1 | 0x01,rx_data,2,100);
	if(status == HAL_OK) {
    // 读取成功

	} else {
		// 处理错误
		Error_Handler();
	}
	return rx_data[0]<<8 | rx_data[1];
}
char Keyboard_GetChar(uint16_t key_data) //解析返回值，返回按下的字符，没有按下就返回\0
{
    // 检查是否只有一个按键按下（只有一个位为1）
    if (key_data == 0 || (key_data & (key_data - 1)) != 0) {
        return '\0';  // 没有按下或多个按下
    }
    
    // 定义按键映射表（从高位到低位）
    const char key_map[] = "0852*741DCBA#963";
    
    // 找到按下的位的位置
    uint8_t pos = 0;
    uint16_t temp = key_data;
    while (temp > 1) {
        temp >>= 1;
        pos++;
    }
    
    // 返回对应的字符
    return key_map[15 - pos];  // 因为高位对应字符串开头，所以要反向
}