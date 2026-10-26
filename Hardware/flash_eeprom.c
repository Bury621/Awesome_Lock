#include "flash_eeprom.h"
#include <string.h>
#include "Config.h"

/**
  * @brief  判断Flash中是否已写入过密码
  * @retval 1: 已写入密码, 0: 未写入密码
  */
uint8_t FLASH_EEPROM_IsPasswordWritten(void)
{
    uint8_t *flash_ptr = (uint8_t*)FLASH_EEPROM_BASE_ADDR;
    
    // 检查Flash是否为擦除状态(全0xFF)
    for(uint16_t i = 0; i < PWD_MAX_LEN; i++)
    {
        if(flash_ptr[i] != 0xFF)
        {
            // 找到非0xFF的数据，说明可能已写入
            return 1;
        }
    }
    
    // 检查是否有有效的字符串(以\0结尾)
    uint16_t i = 0;
    while(i < PWD_MAX_LEN)
    {
        if(flash_ptr[i] == 0xFF)
        {
            // 遇到0xFF，说明后面都是未写入的
            break;
        }
        if(flash_ptr[i] == '\0')
        {
            // 找到有效的字符串结束符
            return 1;
        }
        i++;
    }
    
    return 0;  // 未写入密码
}
HAL_StatusTypeDef FLASH_EEPROM_WritePassword(const char* password)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error;
    
    if(strlen(password) > PWD_MAX_LEN)
        return HAL_ERROR;
    
    // 解锁Flash
    HAL_FLASH_Unlock();
    
    // 直接擦除扇区3
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = PASSWORD_FLASH_SECTOR;//指定扇区
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if(status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }
    
    // 写入数据
    uint8_t *addr = (uint8_t*)FLASH_EEPROM_BASE_ADDR;
    
    for(int i = 0; i < strlen(password); i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)(addr + i), password[i]);
        if(status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }
    
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)(addr + strlen(password)), 0x00);
    if(status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }
    
    HAL_FLASH_Lock();
    return HAL_OK;
}
/**
  * @brief  从Flash EEPROM读取密码
  * @param  password_buffer: 存储读取密码的缓冲区(大小至少为PWD_MAX_LEN+1)
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_EEPROM_ReadPassword(char* password_buffer)
{
    uint8_t *flash_ptr = (uint8_t*)FLASH_EEPROM_BASE_ADDR;
    uint16_t i = 0;
    
    // 从Flash读取数据直到遇到字符串结束符或达到最大长度
    while(i < PWD_MAX_LEN + 1)
    {
        password_buffer[i] = flash_ptr[i];
        if(password_buffer[i] == '\0')
            break;
        i++;
    }
    
    // 确保字符串正确终止
    if(i == PWD_MAX_LEN + 1)
        password_buffer[PWD_MAX_LEN] = '\0';
    
    return HAL_OK;
}