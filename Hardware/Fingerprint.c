#include "Fingerprint.h"

#define PACKAGE_AND_ADDRESS_LEN 6 //包头+地址字节数
#define MAX_PACKET_LEN 64 //数据包缓冲区最大大小

extern UART_HandleTypeDef huart5;

const uint8_t Package_head_and_address[6] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF}; //包头+地址
uint8_t cmd_buffer[MAX_PACKET_LEN]; //打包后的数据包的buffer

// 红色呼吸灯指令
//uint8_t cmd_package_red_light[] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x01,0x04,0x04,0x0A,0x00,0x57};

//这里返回值除了is_successed其他都不可信，不要用，我懒得改而已
FGP_CHECK_RESPONSE FPC_check(void)
{
	uint16_t the_CRC=0;
	//uart_printf("check\r\n");
    //01 00 08 32 50 FF FF 00 00
	FGP_CHECK_RESPONSE result;
    uint8_t cmd[] = {
        0x01, 0x00, 0x08,
        0x32,
        0x50, //分数等级
        0xFF, 0xFF, //id号
        0x00, 0x00 //参数
    };
    uint8_t recv_buffer[17]={0};
    uint8_t len = FPC_Packup(cmd, 9);
    FPC_send_data(cmd_buffer, len);
    // 三次接收，只要最后一次，必须验证包是不是对的
    HAL_UART_Receive(&huart5, recv_buffer, 17, 2000); //1接收
	HAL_UART_Receive(&huart5, recv_buffer, 17, 2000); //2接收
	HAL_UART_Receive(&huart5, recv_buffer, 17, 2000); //3接收
	if(is_from_0(recv_buffer)&& recv_buffer[10]==0x05)
	{
		result.is_successed=recv_buffer[9]==0x00?1:0;
		//uart_printf("\r\nlayin");
		//uart_print_array(recv_buffer,17);
		//uart_printf("\r\nlayin");
	}
	else if(is_from_1(recv_buffer) && recv_buffer[11]==0x05)
	{
		result.is_successed=recv_buffer[10]==0x00?1:0;
		//uart_printf("\r\nlayout");
		//uart_print_array(recv_buffer,17);
		//uart_printf("layout");
	}
	else
	{
		result.is_successed=0; //数据不可信
	}
    //uart_print_array(recv_buffer,17); //调试输出
    return result;
}


void FPC_clear(void)
{
	//uart_printf("clearing\r\n");
	uint8_t recv_buffer[20];
	uint8_t cmd[]={0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x0d,0x00,0x11};
	FPC_send_data(cmd,12);
	
	HAL_UART_Receive(&huart5,recv_buffer,16,200);
	
	//uart_printf("Clear log\r\n");
	//uart_print_array(recv_buffer,12);
}
uint32_t FPC_Get_Registed(void)
{
    uint8_t len;
    uint8_t recv_buffer[44] = {0};
    uint32_t result = 0;
    
    uint8_t cmd1[] = {0x01, 0x00, 0x04, 0x1F, 0x00};
    len = FPC_Packup(cmd1, 5);
    FPC_send_data(cmd_buffer, len);
    //uart_print_array(cmd_buffer,len);
	
	
    HAL_UART_Receive(&huart5, recv_buffer, 44, 2000);
	if(is_from_0(recv_buffer))
	{
    // 正确的字节顺序：低位字节对应小ID
		result = ((uint32_t)recv_buffer[10] << 0)   |  // ID 0-7   (bit0=ID0)
				 ((uint32_t)recv_buffer[11] << 8)   |  // ID 8-15  (bit0=ID8)
				 ((uint32_t)recv_buffer[12] << 16)  |  // ID 16-23 (bit0=ID16)
				 ((uint32_t)recv_buffer[13] << 24);    // ID 24-31 (bit0=ID24)
	}
	else if(is_from_1(recv_buffer))
	{
		    // 正确的字节顺序：低位字节对应小ID
		result = ((uint32_t)recv_buffer[11] << 0)   |  // ID 0-7   (bit0=ID0)
				 ((uint32_t)recv_buffer[12] << 8)   |  // ID 8-15  (bit0=ID8)
				 ((uint32_t)recv_buffer[13] << 16)  |  // ID 16-23 (bit0=ID16)
				 ((uint32_t)recv_buffer[14] << 24);    // ID 24-31 (bit0=ID24)
	}
	else
	{
		result=0xFFFFFF; //返回的东西不对
	}
    
	//uart_print_array(recv_buffer,44);
    //uart_printf("Confim code: %#02X, index: %#08lX\r\n", recv_buffer[9], result);
    return result;
}

/*
打包数据包
Read_cmd指令
len指令的长度
返回值是cmd_buffer中实际有的数据长度
*/
uint8_t FPC_Packup(uint8_t Read_cmd[], uint8_t len)
{
    uint8_t *p; //一个指向cmd_buffer的指针,用来写入
    uint16_t the_CRC;
    p = cmd_buffer;
    
    for(uint8_t x = 0; x < PACKAGE_AND_ADDRESS_LEN; x++) //写入包头+地址
    {
        *p = Package_head_and_address[x]; //写入
        p++;
    }
    
    for(uint8_t x = 0; x < len; x++)//写入命令数组
    {
        *p = Read_cmd[x];
        p++;
    }
    
    the_CRC = FPC_get_CRC(Read_cmd, len); //16位校验和
    *p = the_CRC >> 8; //取高八位写入
    p++;
    *p = the_CRC & 0xFF; //取低八位写入
    
    return PACKAGE_AND_ADDRESS_LEN + len + 2; //返回数据包总长度
}

uint16_t FPC_get_CRC(uint8_t cmd[], uint8_t len) //传入命令数组，不包含包头和地址
{
    uint16_t sum = 0; //初始化校验和
    for(uint8_t x = 0; x < len; x++)
    {
        sum += cmd[x];
    }
    return sum;
}

void FPC_send_data(uint8_t datas[], uint8_t len)
{
    HAL_UART_Transmit(&huart5, datas, len, 100);
}
FGP_Status_Code FPC_register(void)
{
    uint32_t registered_data = FPC_Get_Registed(); //获取索引情况
    uint8_t recv_buffer[14];
    
	//print_binary16(registered_data);
    //uart_printf("\r\n");
    
    for(uint8_t x = 0; x < 32; x++)
    {
        if(((registered_data >> x) & 0x01) == 0)
        {
            //uart_printf("Attempting to register at ID%d\r\n", x);
            
            uint8_t cmd[] = {
                0x01, 0x00, 0x08, //包头+包长
                0x31, //指令码
                (uint8_t)(x >> 8), (uint8_t)(x & 0xFF),  //id号
                FGP_COLLECT_FINGER_NUMBER, //录入次数
                0x00, 0x1C |  FGP_ALLOW_FAINGER_OUT <<5 | FGP_FINGER_RESGIST_LIGHT //参数
            };
            uint8_t len = FPC_Packup(cmd, 9);
            FPC_send_data(cmd_buffer, len);
			HAL_UART_Receive(&huart5, recv_buffer, 14, 100000);
			FGP_Status_Code status;
            if(is_from_0(recv_buffer)){
				status = (FGP_Status_Code)recv_buffer[9];
			}
			else if (is_from_1(recv_buffer)){
				status = (FGP_Status_Code)recv_buffer[10];
			}
			else{
				status=0xFF; //乱包
			}
            //uart_printf("Registration result: 0x%02X\r\n", status);
            //uart_printf("=== Registration End ===\r\n\r\n");
            
			//uart_printf("Recvive_from_module\r\n");
			//uart_print_array(recv_buffer,14);
			//uart_printf("Register status:0x%02X\r\n",status);
            return status;
        }
    }
    return FGP_STATUS_LIBRARY_FULL;
}

/**
 * @brief 控制指纹模块灯光
 * @param color: 灯光颜色 (对于呼吸灯是结束颜色，其他功能与起始颜色一致)
 * @param light_status: 灯光功能模式
 * @param start_color: 起始颜色 (主要用于呼吸灯)
 * @param cycle: 循环次数 (0=无限循环)
 * @param time: 时间参数 (单位0.1秒)
 */
void FGP_Control_light(LIGHT_COLOR color, LIGHT_STATUS light_status, LIGHT_COLOR start_color, uint8_t cycle, uint8_t time)
{
    uint8_t cmd[] = {
        0x01, 0x00, 0x08,     // 包头标识 + 包长度(8字节数据)
        0x3C,                 // 指令码
        (uint8_t)light_status,  // 功能码 (您的枚举从0开始，协议从1开始)
        start_color,          // 起始颜色
        color,                // 结束颜色
        time,                 // 时间参数
        cycle                 // 循环次数
    };
    
    uint8_t len = FPC_Packup(cmd, 9);  // 9个字节数据
    FPC_send_data(cmd_buffer, len);
}

// 常用功能的封装函数
void FGP_Breath_light(LIGHT_COLOR start_color, LIGHT_COLOR end_color, uint8_t cycle, uint8_t time)
{
    FGP_Control_light(end_color, FGP_LIGHT_BREATHE, start_color, cycle, time);
}

void FGP_Blink_light(LIGHT_COLOR color, uint8_t cycle, uint8_t time)
{
    FGP_Control_light(color, FGP_LIGHT_FLASH, color, cycle, time);
}

void FGP_Constant_light(LIGHT_COLOR color)
{
    FGP_Control_light(color, FGP_LIGHT_OPEN, color, 0, 0);
}

void FGP_Turn_off_light(void)
{
    FGP_Control_light(FGP_LIGHT_OFF, FGP_LIGHT_CLOSE, FGP_LIGHT_OFF, 0, 0);
}

void FGP_Fade_in_light(LIGHT_COLOR color, uint8_t cycle, uint8_t time)
{
    FGP_Control_light(color, FGP_LIGHT_SLOW_OPEN, color, cycle, time);
}

void FGP_Fade_out_light(LIGHT_COLOR color, uint8_t cycle, uint8_t time)
{
    FGP_Control_light(color, FGP_LIGHT_SLOW_CLOSE, color, cycle, time);
}
bool is_from_0(uint8_t buffer[])
{
	for(uint8_t x=0;x<PACKAGE_AND_ADDRESS_LEN;x++)
	{
		if(buffer[x]!=Package_head_and_address[x])
		{
			return 0; //包头不一样返回0
		}
	}
	return 1;
}
bool is_from_1(uint8_t buffer[])
{
	for(uint8_t x=1;x<=PACKAGE_AND_ADDRESS_LEN;x++)
	{
		if(buffer[x]!=Package_head_and_address[x-1])
		{
			return 0; //包头不一样返回0
		}
	}
	return 1;
}