
#include "tuya_cloud_types.h"
#include "tkl_flash.h"

#include <common/bk_typedef.h>
#include "driver/flash.h"

typedef struct
{
    BOOL            is_start;
    unsigned long   set_ms;
    unsigned long   start;
    unsigned long   current;
} TUYA_OS_STORAGE_TIMER;
//flash 最大持续处理时间
#define FLASH_MAX_HANDLE_KEEP_TIME 10000    //10s

/* TODO: need to consider whether to use locks at the TKL layer*/
//extern int hal_flash_lock(void);
//extern int hal_flash_unlock(void);
extern void flash_lock(void);
extern void flash_unlock(void);

// total: 800000
// Name     Begin       End         Length
// boot     0x0         0x11000     68k
// cpu0     0x11000     0x20f000    2040k
// cpu1     0x20f000    0x31f000    1088k
// ota1     0x31f000    0x7cb000    4784k
// ota0     0x7cb000    0x7cd000    8k
// kvp      0x7cd000    0x7ce000    4k
// res      0x7ce000    0x7dd000    60k
// kv       0x7dd000    0x7ed000    64k
// uf       0x7ed000    0x7fd000    64k
// key      0x7fd000    0x7fe000    4k
// rf       0x7fe000    0x7ff000    4k
// net      0x7ff000    0x800000    4k

#define PARTITION_SIZE         (1 << 12) /* 4KB */
#define FLH_BLOCK_SZ            PARTITION_SIZE


#define APPLICATION_START               0x10000
#define APPLICATION_SIZE                (4784 * 1024)

#define OTA_START                       0x7cb000
#define OTA_SIZE                        (8 * 1024)

#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
    #define TUYA_KV_PROTECTED           0x7cd000
    #define TUYA_KV_PROTECTED_SIZE      4096
#endif

#define TUYA_USER1                      0x7ce000
#define TUYA_USER1_SIZE                 (32 * 1024)

#define TUYA_USER2                      0x7d6000
#define TUYA_USER2_SIZE                 (28 * 1024)

#define TUYA_KV                         0x7dd000
#define TUYA_KV_SIZE                    (64 * 1024)

#define TUYA_UF                         0x7ed000
#define TUYA_UF_SIZE                    (64 * 1024)

#define TUYA_KV_KEY                     0x7fd000
#define TUYA_KV_KEY_SIZE                4096

#define FACTORY_RF_DATA_START           0x7fe000
#define FACTORY_RF_DATA_SIZE            4096

#define FACTORY_FAST_CONNECT_DATA_START 0x7ff000
#define FACTORY_FAST_CONNECT_DATA_SIZE  4096

/**
 * @brief flash 设置保护,enable 设置ture为全保护，false为半保护
 *
 * @return OPRT_OK
 */
int tkl_flash_set_protect(const BOOL enable)
{
    unsigned int  param;

    if (enable) {
        param = FLASH_PROTECT_ALL;
        bk_flash_set_protect_type(param);
    } else {
        param = FLASH_PROTECT_NONE;
        bk_flash_set_protect_type(param);
    }

    return OPRT_OK;
}


/**
* @brief read flash
*
* @param[in] addr: flash address
* @param[out] dst: pointer of buffer
* @param[in] size: size of buffer
*
* @note This API is used for reading flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_read(uint32_t addr, UCHAR_T *dst, uint32_t size)
{
    if (NULL == dst) {
        return OPRT_INVALID_PARM;
    }

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_lock();

    bk_flash_read_bytes(addr, (uint8_t *)dst, size);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_unlock();

    return OPRT_OK;
}

static unsigned int __uni_flash_is_protect_all(void)
{
    unsigned int param;
    param = bk_flash_get_protect_type();
    return (FLASH_PROTECT_ALL == param || FLASH_UNPROTECT_LAST_BLOCK == param);
}

/**
* @brief write flash
*
* @param[in] addr: flash address
* @param[in] src: pointer of buffer
* @param[in] size: size of buffer
*
* @note This API is used for writing flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_write(uint32_t addr, const UCHAR_T *src, uint32_t size)
{
    unsigned int protect_flag;
    unsigned int param;

    if (NULL == src) {
        return OPRT_INVALID_PARM;
    }

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_lock();

#if 0
    //解保护
    protect_flag = __uni_flash_is_protect_all();

    if (protect_flag) {
        param = FLASH_PROTECT_HALF;
        bk_flash_set_protect_type(param);
    }
#endif

    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    bk_flash_write_bytes(addr, (const uint8_t *)src, size);
    bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_unlock();

    return OPRT_OK;
}

/**
* @brief erase flash
*
* @param[in] addr: flash address
* @param[in] size: size of flash block
*
* @note This API is used for erasing flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_erase(uint32_t addr, uint32_t size)
{
    unsigned short start_sec = (addr / PARTITION_SIZE);
    unsigned short end_sec = ((addr + size - 1) / PARTITION_SIZE);
    // unsigned int status;
    unsigned int i = 0;
    unsigned int sector_addr;
    // DD_HANDLE flash_handle;
    unsigned int  param;
    unsigned int protect_flag;

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_lock();

    // TODO flash unprotect
#if 0
    //解保护
    protect_flag = __uni_flash_is_protect_all();
    if (protect_flag) {
        param = FLASH_PROTECT_HALF;
        bk_flash_set_protect_type(param);
    }
#endif

    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    for (i = start_sec; i <= end_sec; i++) {
        sector_addr = PARTITION_SIZE * i;
        bk_flash_erase_sector(sector_addr);
    }
    bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

#if 0
    protect_flag = __uni_flash_is_protect_all();
    if(protect_flag)
    {
        param = FLASH_PROTECT_ALL;
        bk_flash_set_protect_type(param);
    }
#endif

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_unlock();

    return OPRT_OK;
}

/**
* @brief lock flash
*
* @param[in] addr: lock begin addr
* @param[in] size: lock area size
*
* @note This API is used for lock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_lock(uint32_t addr, uint32_t size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief unlock flash
*
* @param[in] addr: unlock begin addr
* @param[in] size: unlock area size
*
* @note This API is used for unlock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_unlock(uint32_t addr, uint32_t size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief get flash information
*
* @param[out] info: the description of the flash
*
* @note This API is used to get description of storage.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_E type, TUYA_FLASH_BASE_INFO_T* info)
{
    if ((type > TUYA_FLASH_TYPE_MAX) || (info == NULL)) {
        return OPRT_INVALID_PARM;
    }
    switch (type) {
        case TUYA_FLASH_TYPE_UF:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = TUYA_UF;
            info->partition[0].size = TUYA_UF_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_DATA:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = TUYA_KV;
            info->partition[0].size = TUYA_KV_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_KEY:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = TUYA_KV_KEY;
            info->partition[0].size = TUYA_KV_KEY_SIZE;
            break;
#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
        case TUYA_FLASH_TYPE_KV_PROTECT:
            info->partition_num = 1;
            info->partition[0].block_size = TUYA_KV_PROTECTED_SIZE;
            info->partition[0].start_addr = TUYA_KV_PROTECTED;
            info->partition[0].size = TUYA_KV_PROTECTED_SIZE;
            break;
#endif
        case TUYA_FLASH_TYPE_APP:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = APPLICATION_START;
            info->partition[0].size = APPLICATION_SIZE;
            break;
        case TUYA_FLASH_TYPE_APP_BIN:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = APPLICATION_START;
            info->partition[0].size = APPLICATION_SIZE;
            break;
        case TUYA_FLASH_TYPE_OTA:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = OTA_START;
            info->partition[0].size = OTA_SIZE;
            break;
        case TUYA_FLASH_TYPE_USER0:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = TUYA_USER1;
            info->partition[0].size = TUYA_USER1_SIZE;
            break;
        case TUYA_FLASH_TYPE_USER1:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = TUYA_USER2;
            info->partition[0].size = TUYA_USER2_SIZE;
            break;

        default:
            return OPRT_INVALID_PARM;
            break;
    }

    return OPRT_OK;

}
