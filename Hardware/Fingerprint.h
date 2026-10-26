#ifndef __FINGERPRINT_H
#define __FINGERPRINT_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/* 指纹模块返回状态码 */
typedef enum
{
    FGP_STATUS_OK                      = 0x00,
    FGP_STATUS_PACKET_ERROR            = 0x01,
    FGP_STATUS_NO_FINGER               = 0x02,
    FGP_STATUS_IMAGE_FAIL              = 0x03,
    FGP_STATUS_IMAGE_TOO_DRY           = 0x04,
    FGP_STATUS_IMAGE_TOO_WET           = 0x05,
    FGP_STATUS_IMAGE_TOO_MESSY         = 0x06,
    FGP_STATUS_FEW_FEATURES            = 0x07,
    FGP_STATUS_NOT_MATCH               = 0x08,
    FGP_STATUS_NOT_FOUND               = 0x09,
    FGP_STATUS_MERGE_FAIL              = 0x0A,
    FGP_STATUS_ADDR_OUT_OF_RANGE       = 0x0B,
    FGP_STATUS_READ_TEMPLATE_ERROR     = 0x0C,
    FGP_STATUS_UPLOAD_FEATURE_FAIL     = 0x0D,
    FGP_STATUS_CANNOT_RECEIVE_DATA     = 0x0E,
    FGP_STATUS_UPLOAD_IMAGE_FAIL       = 0x0F,
    FGP_STATUS_DELETE_FAIL             = 0x10,
    FGP_STATUS_CLEAR_LIB_FAIL          = 0x11,
    FGP_STATUS_CANNOT_ENTER_LOW_POWER  = 0x12,
    FGP_STATUS_PASSWORD_ERROR          = 0x13,
    FGP_STATUS_RESET_FAIL              = 0x14,
    FGP_STATUS_NO_VALID_IMAGE          = 0x15,
    FGP_STATUS_RESIDUAL_FINGER         = 0x17,
    FGP_STATUS_FLASH_ERROR             = 0x18,
    FGP_STATUS_INVALID_REGISTER        = 0x1A,
    FGP_STATUS_REGISTER_CONTENT_ERROR  = 0x1B,
    FGP_STATUS_NOTEBOOK_PAGE_ERROR     = 0x1C,
    FGP_STATUS_PORT_OPERATION_FAIL     = 0x1D,
    FGP_STATUS_AUTO_ENROLL_FAIL        = 0x1E,
    FGP_STATUS_LIBRARY_FULL            = 0x1F,
    FGP_STATUS_DEVICE_ADDR_ERROR       = 0x20,
    FGP_STATUS_PASSWORD_INVALID        = 0x21,
    FGP_STATUS_TEMPLATE_NOT_EMPTY      = 0x22,
    FGP_STATUS_TEMPLATE_EMPTY          = 0x23,
    FGP_STATUS_LIBRARY_EMPTY           = 0x24,
    FGP_STATUS_ENROLL_COUNT_ERROR      = 0x25,
    FGP_STATUS_TIMEOUT                 = 0x26,
    FGP_STATUS_FINGER_EXISTS           = 0x27,
    FGP_STATUS_TEMPLATE_ASSOCIATED     = 0x28,
    FGP_STATUS_SENSOR_INIT_FAIL        = 0x29,
    FGP_STATUS_MODULE_INFO_NOT_EMPTY   = 0x2A,
    FGP_STATUS_MODULE_INFO_EMPTY       = 0x2B,
    FGP_STATUS_IMAGE_AREA_SMALL        = 0x33,
    FGP_STATUS_IMAGE_NOT_USABLE        = 0x34,
    FGP_STATUS_INVALID_DATA            = 0x35,
    FGP_STATUS_ENROLL_COUNT_TOO_FEW    = 0x40
} FGP_Status_Code;

/* 指纹操作类型 */
typedef enum
{
    FPC_OP_NONE = 0,
    FPC_OP_VERIFY,
    FPC_OP_ENROLL,
    FPC_OP_CLEAR
} FPC_Op;

/* 操作结果 */
typedef enum
{
    FPC_RESULT_OK = 0,
    FPC_RESULT_FAIL,
    FPC_RESULT_TIMEOUT,
    FPC_RESULT_COMM_ERROR
} FPC_Result;

/* 灯光模式（内部会转换为模块指令） */
typedef enum
{
    FPC_LIGHT_OFF = 0,
    FPC_LIGHT_BLUE_BREATH,
    FPC_LIGHT_RED_CONST,
    FPC_LIGHT_GREEN_CONST
} FPC_LightMode;

/* 事件回调：在主循环上下文（FPC_Poll 内）被调用 */
typedef void (*FPC_EventHandler)(FPC_Op op, FPC_Result result, FGP_Status_Code status);

/*
 * 初始化指纹模块通信。
 * 使用 UART5 接收中断 + 帧解析状态机，不再阻塞等待。
 */
void FPC_Init(FPC_EventHandler handler);

/* 发起操作；模块忙时返回 false */
bool FPC_StartVerify(void);
bool FPC_StartEnroll(void);
bool FPC_StartClear(void);

/* 取消当前等待（例如用户返回主界面） */
void FPC_Cancel(void);

/* 主循环轮询：处理超时、延迟的灯光指令、UART 接收恢复 */
void FPC_Poll(void);

bool FPC_IsBusy(void);

/* 设置模块灯光；若模块忙则延迟到空闲时发送，避免 ACK 串包 */
void FPC_SetLight(FPC_LightMode mode);

#endif /* __FINGERPRINT_H */
