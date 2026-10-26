#ifndef __FLASH_EEPROM_H
#define __FLASH_EEPROM_H

#include "main.h"
#include <stdbool.h>

/*
 * 密码存储（STM32F4 内部 Flash 模拟 EEPROM）
 *
 * 旧版问题：
 *   - 只写原始字符串，无魔数/长度/校验，Flash 读回 0xFF 时会被当成有效密码；
 *   - 写入边界判断 `strlen(pwd) > PWD_MAX_LEN` 允许 32 字符 + '\0'，多写 1 字节；
 *   - 擦除/编程期间没有针对看门狗做任何处理；
 *   - 存储地址 0x0800C000 只有 16KB，固件一旦超过 48KB 就会被自己擦掉。
 *
 * 新版：带魔数、版本、长度、CRC32 的记录；启动时校验，损坏则回退默认密码；
 *       地址改到 Sector5（0x08020000），给固件留出 128KB 空间。
 */
void FLASH_EEPROM_Init(void);

/* 永远返回有效字符串；未保存过或记录损坏时返回默认密码 */
const char *FLASH_EEPROM_GetPassword(void);

/* 保存新密码：仅允许 1..PWD_MAX_LEN-1 位数字。成功返回 true */
bool FLASH_EEPROM_SetPassword(const char *password);

/* 当前密码是否为出厂默认值（用于界面提示） */
bool FLASH_EEPROM_IsDefault(void);

#endif /* __FLASH_EEPROM_H */
