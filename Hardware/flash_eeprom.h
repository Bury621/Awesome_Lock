#ifndef __FLASH_EEPROM_H
#define __FLASH_EEPROM_H

#include "stm32f4xx_hal.h"

// 函数声明
HAL_StatusTypeDef FLASH_EEPROM_WritePassword(const char* password);
HAL_StatusTypeDef FLASH_EEPROM_ReadPassword(char* password_buffer);
uint8_t FLASH_EEPROM_IsPasswordWritten(void);

#endif /* __FLASH_EEPROM_H */