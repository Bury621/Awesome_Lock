#ifndef __FINGERPRINT_H
#define __FINGERPRINT_H
#include "main.h"
#include "Debug.h"
#include <stdbool.h>
#include "Config.h"
#include <string.h>

typedef enum { //状态枚举
    FGP_STATUS_OK                      = 0x00, // 指令执行完毕或 OK
    FGP_STATUS_PACKET_ERROR            = 0x01, // 数据包接收错误
    FGP_STATUS_NO_FINGER               = 0x02, // 传感器上没有手指
    FGP_STATUS_IMAGE_FAIL              = 0x03, // 录入指纹图像失败
    FGP_STATUS_IMAGE_TOO_DRY           = 0x04, // 指纹图像太干、太淡而生不成特征
    FGP_STATUS_IMAGE_TOO_WET           = 0x05, // 指纹图像太湿、太糊而生不成特征
    FGP_STATUS_IMAGE_TOO_MESSY         = 0x06, // 指纹图像太乱而生不成特征
    FGP_STATUS_FEW_FEATURES            = 0x07, // 指纹图像正常，但特征点太少（或面积太小）而生不成特征
    FGP_STATUS_NOT_MATCH               = 0x08, // 指纹不匹配
    FGP_STATUS_NOT_FOUND               = 0x09, // 没搜索到指纹
    FGP_STATUS_MERGE_FAIL              = 0x0A, // 特征合并失败
    FGP_STATUS_ADDR_OUT_OF_RANGE       = 0x0B, // 访问指纹库时地址序号超出指纹库范围
    FGP_STATUS_READ_TEMPLATE_ERROR     = 0x0C, // 从指纹库读模板出错或无效
    FGP_STATUS_UPLOAD_FEATURE_FAIL     = 0x0D, // 上传特征失败
    FGP_STATUS_CANNOT_RECEIVE_DATA     = 0x0E, // 模组不能接收后续数据包
    FGP_STATUS_UPLOAD_IMAGE_FAIL       = 0x0F, // 上传图像失败
    FGP_STATUS_DELETE_FAIL             = 0x10, // 删除模板失败
    FGP_STATUS_CLEAR_LIB_FAIL          = 0x11, // 清空指纹库失败
    FGP_STATUS_CANNOT_ENTER_LOW_POWER  = 0x12, // 不能进入低功耗状态
    FGP_STATUS_PASSWORD_ERROR          = 0x13, // 口令不正确
    FGP_STATUS_RESET_FAIL              = 0x14, // 系统复位失败
    FGP_STATUS_NO_VALID_IMAGE          = 0x15, // 缓冲区内没有有效原始图而生不成图像
    FGP_STATUS_RESIDUAL_FINGER         = 0x17, // 残留指纹或两次采集之间手指没有移动过
    FGP_STATUS_FLASH_ERROR             = 0x18, // 读写 FLASH 出错
    FGP_STATUS_INVALID_REGISTER        = 0x1A, // 无效寄存器号
    FGP_STATUS_REGISTER_CONTENT_ERROR  = 0x1B, // 寄存器设定内容错误号
    FGP_STATUS_NOTEBOOK_PAGE_ERROR     = 0x1C, // 记事本页码指定错误
    FGP_STATUS_PORT_OPERATION_FAIL     = 0x1D, // 端口操作失败
    FGP_STATUS_AUTO_ENROLL_FAIL        = 0x1E, // 自动注册（enroll）失败
    FGP_STATUS_LIBRARY_FULL            = 0x1F, // 指纹库满
    FGP_STATUS_DEVICE_ADDR_ERROR       = 0x20, // 设备地址错误
    FGP_STATUS_PASSWORD_INVALID        = 0x21, // 密码有误
    FGP_STATUS_TEMPLATE_NOT_EMPTY      = 0x22, // 指纹模板非空
    FGP_STATUS_TEMPLATE_EMPTY          = 0x23, // 指纹模板为空
    FGP_STATUS_LIBRARY_EMPTY           = 0x24, // 指纹库为空
    FGP_STATUS_ENROLL_COUNT_ERROR      = 0x25, // 录入次数设置错误
    FGP_STATUS_TIMEOUT                 = 0x26, // 超时
    FGP_STATUS_FINGER_EXISTS           = 0x27, // 指纹已存在
    FGP_STATUS_TEMPLATE_ASSOCIATED     = 0x28, // 指纹模板有关联
    FGP_STATUS_SENSOR_INIT_FAIL        = 0x29, // 传感器初始化失败
    FGP_STATUS_MODULE_INFO_NOT_EMPTY   = 0x2A, // 模组信息非空
    FGP_STATUS_MODULE_INFO_EMPTY       = 0x2B, // 模组信息为空
    FGP_STATUS_IMAGE_AREA_SMALL        = 0x33, // 图像面积小
    FGP_STATUS_IMAGE_NOT_USABLE        = 0x34, // 图像不可用
    FGP_STATUS_INVALID_DATA            = 0x35, // 非法数据
    FGP_STATUS_ENROLL_COUNT_TOO_FEW    = 0x40  // 注册次数少于规定次数
} FGP_Status_Code; //状态枚举

typedef enum {
    FGP_REGIST_STATUS_ERROR   = -1, // 通信错误或指令失败
    FGP_REGIST_STATUS_EMPTY   = 0,  // 未注册，可以注册
    FGP_REGIST_STATUS_EXISTS  = 1   // 已注册，不能重复注册
} FGP_Regist_Status;

typedef enum {
    FGP_LIGHT_OFF    = 0x00,  // 全灭
    FGP_LIGHT_BLUE   = 0x01,  // 蓝灯
    FGP_LIGHT_GREEN  = 0x02,  // 绿灯
    FGP_LIGHT_CYAN   = 0x03,  // 青灯(绿+蓝)
    FGP_LIGHT_RED    = 0x04,  // 红灯
    FGP_LIGHT_PURPLE = 0x05,  // 紫灯(红+蓝)
    FGP_LIGHT_YELLOW = 0x06,  // 黄灯(红+绿)
    FGP_LIGHT_WHITE  = 0x07   // 白灯(红+绿+蓝)
} LIGHT_COLOR;

typedef enum {
    FGP_LIGHT_BREATHE    = 0x01, // 呼吸灯
    FGP_LIGHT_FLASH      = 0x02, // 闪烁灯
    FGP_LIGHT_OPEN       = 0x03, // 常开灯
    FGP_LIGHT_CLOSE      = 0x04, // 常闭灯
    FGP_LIGHT_SLOW_OPEN  = 0x05, // 渐开灯
    FGP_LIGHT_SLOW_CLOSE = 0x06, // 渐灭灯
    FGP_LIGHT_RUNNING    = 0x07  // 跑马灯
} LIGHT_STATUS;

typedef struct {
    bool is_successed;          // 是否找到
} FGP_CHECK_RESPONSE;

// 函数声明
void FPC_send_data(uint8_t datas[], uint8_t len);
uint16_t FPC_get_CRC(uint8_t cmd[], uint8_t len);
uint8_t FPC_Packup(uint8_t Read_cmd[], uint8_t len);
uint32_t FPC_Get_Registed(void);
FGP_CHECK_RESPONSE FPC_check(void);
void FPC_clear(void);
FGP_Status_Code FPC_register(void);

// 灯光控制函数
void FGP_Control_light(LIGHT_COLOR color, LIGHT_STATUS light_status, LIGHT_COLOR start_color, uint8_t cycle, uint8_t time);
void FGP_Breath_light(LIGHT_COLOR start_color, LIGHT_COLOR end_color, uint8_t cycle, uint8_t time);
void FGP_Blink_light(LIGHT_COLOR color, uint8_t cycle, uint8_t time);
void FGP_Constant_light(LIGHT_COLOR color);
void FGP_Turn_off_light(void);
void FGP_Fade_in_light(LIGHT_COLOR color, uint8_t cycle, uint8_t time);
void FGP_Fade_out_light(LIGHT_COLOR color, uint8_t cycle, uint8_t time);
bool is_from_0(uint8_t buffer[]);
bool is_from_1(uint8_t buffer[]);
#endif