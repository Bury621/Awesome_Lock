#include "flash_eeprom.h"
#include "app_util.h"
#include <stddef.h>

extern IWDG_HandleTypeDef hiwdg;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    uint16_t length;
    char     password[PWD_MAX_LEN];
    uint32_t crc;
} PwdRecord;

/* 记录大小必须是 4 的倍数，才能按字编程 */
typedef char PwdRecordSizeCheck[(sizeof(PwdRecord) % 4U) == 0U ? 1 : -1];

static char s_password[PWD_MAX_LEN + 1U];
static bool s_is_default;

static uint32_t flash_crc32(const void *data, uint32_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t i = 0U; i < length; i++)
    {
        crc ^= bytes[i];
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFFU;
}

static bool password_is_digits(const char *password, size_t length)
{
    if ((password == NULL) || (length == 0U) || (length >= PWD_MAX_LEN))
    {
        return false;
    }

    for (size_t i = 0U; i < length; i++)
    {
        if ((password[i] < '0') || (password[i] > '9'))
        {
            return false;
        }
    }

    return true;
}

static bool record_is_valid(const PwdRecord *record)
{
    PwdRecord copy;
    uint32_t crc;

    if (record == NULL)
    {
        return false;
    }
    if ((record->magic != PWD_STORE_MAGIC) ||
        (record->version != PWD_STORE_VERSION) ||
        (record->length == 0U) ||
        (record->length >= PWD_MAX_LEN))
    {
        return false;
    }

    copy = *record;
    copy.crc = 0U;
    crc = flash_crc32(&copy, offsetof(PwdRecord, crc));
    if (crc != record->crc)
    {
        return false;
    }

    return password_is_digits(record->password, record->length) &&
           (record->password[record->length] == '\0');
}

void FLASH_EEPROM_Init(void)
{
    const PwdRecord *record = (const PwdRecord *)(uintptr_t)FLASH_EEPROM_BASE_ADDR;

    if (record_is_valid(record))
    {
        (void)memcpy(s_password, record->password, PWD_MAX_LEN);
        s_password[PWD_MAX_LEN] = '\0';
        s_is_default = false;
    }
    else
    {
        (void)strncpy(s_password, DEFULT_PWD, PWD_MAX_LEN);
        s_password[PWD_MAX_LEN] = '\0';
        s_is_default = true;
    }
}

const char *FLASH_EEPROM_GetPassword(void)
{
    return s_password;
}

bool FLASH_EEPROM_IsDefault(void)
{
    return s_is_default;
}

bool FLASH_EEPROM_SetPassword(const char *password)
{
    static PwdRecord record;
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sector_error = 0U;
    uint32_t address = (uint32_t)FLASH_EEPROM_BASE_ADDR;
    bool ok = true;
    size_t length;

    if (password == NULL)
    {
        return false;
    }

    length = strlen(password);
    if ((length < PWD_MIN_LEN) || !password_is_digits(password, length))
    {
        return false;
    }

    (void)memset(&record, 0, sizeof(record));
    record.magic = PWD_STORE_MAGIC;
    record.version = PWD_STORE_VERSION;
    record.length = (uint16_t)length;
    (void)memcpy(record.password, password, length);
    record.password[length] = '\0';
    record.crc = flash_crc32(&record, offsetof(PwdRecord, crc));

    /*
     * Flash 擦除/编程期间 CPU 取指会被挂起，中断也无法及时执行。
     * 先喂狗，再关中断完成整个“擦除 + 写入 + 校验”，确保不超过 IWDG 周期。
     * 本工程 IWDG 约 10s，Sector5（128KB）最坏擦除时间约 4s，余量足够。
     */
    (void)HAL_IWDG_Refresh(&hiwdg);

    uint32_t primask = App_EnterCritical();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = PASSWORD_FLASH_SECTOR;
    erase.NbSectors = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)
    {
        ok = false;
    }

    if (ok)
    {
        const uint8_t *source = (const uint8_t *)&record;

        for (uint32_t offset = 0U; offset < sizeof(record); offset += 4U)
        {
            uint32_t word;

            (void)memcpy(&word, &source[offset], sizeof(word));
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                  address + offset, word) != HAL_OK)
            {
                ok = false;
                break;
            }
        }
    }

    HAL_FLASH_Lock();

    /* 回读校验，避免“返回成功但实际写坏” */
    if (ok)
    {
        const PwdRecord *written = (const PwdRecord *)(uintptr_t)FLASH_EEPROM_BASE_ADDR;
        if (memcmp(written, &record, sizeof(record)) != 0)
        {
            ok = false;
        }
    }

    App_ExitCritical(primask);

    if (ok)
    {
        (void)memcpy(s_password, record.password, PWD_MAX_LEN);
        s_password[PWD_MAX_LEN] = '\0';
        s_is_default = false;
    }

    return ok;
}
