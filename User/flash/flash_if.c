/**
 * @brief   FLASH接口
 */

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

#include <string.h>

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

/* 用户自定义的模块头文件 (User/Application Headers) */

#include "flash_if.h"

/* ==================== Private Constants & Defines ================ */
/* 💡 这里放：仅限本文件内部使用的宏定义、私有常量 */

#define FLASH_IF_INVALID_SECTOR 0xFFU

/* ==================== Private Types ============================== */
/* 💡 这里放：仅限本文件内部使用、不需要暴露给外部的私有结构体或联合体定义 */

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Private Variables ========================== */
/* 💡 这里放：本文件内部使用的静态全局变量 */

/* ==================== Private Declarations ======================= */
/* 💡 这里放：本文件内部 static 辅助函数的声明 */

static uint32_t FlashIf_GetSector(uint32_t Address);
static HAL_StatusTypeDef FlashIf_CheckRange(uint32_t start_addr, uint32_t data_size);

/* ==================== Public APIs ================================ */
/* 💡 这里放：对外公开接口的具体实现 */

/**
  * @brief  初始化 Flash 接口并清除所有错误状态标志位
  * @param  None
  * @retval None
  */
void FlashIf_Init(void)
{
    /* 解锁 Flash 以允许清除标志位 */
    HAL_FLASH_Unlock();

    /* 清除所有 FLASH 错误及操作完成标志位 */
    __HAL_FLASH_CLEAR_FLAG(
        FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
        FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    /* 重新锁定 Flash */
    HAL_FLASH_Lock();
}

/**
 * @brief  FLASH 擦除程序
 * @param  start: 擦除起始地址
 * @param  data_size: 即将写入的数据大小，单位字节
 * @retval HAL_OK    : 擦除成功
 *         HAL_ERROR : 擦除失败
 */
HAL_StatusTypeDef FlashIf_Erase(uint32_t start_addr, uint32_t data_size)
{
    FLASH_EraseInitTypeDef erase        = {0};
    uint32_t               sector_error = 0;
    // 起始与结束扇区
    uint32_t               start_sector, end_sector;

    if (FlashIf_CheckRange(start_addr, data_size) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // 计算起始与结束扇区
    start_sector = FlashIf_GetSector(start_addr);
    end_sector   = FlashIf_GetSector(start_addr + data_size - 1U);

    // 配置擦除参数：按扇区擦除
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;

    // 工作电压范围：2.7V - 3.6V
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // 起始扇区号
    erase.Sector = start_sector;

    // 需要擦除的扇区总数
    erase.NbSectors = end_sector - start_sector + 1U;

    // 解锁 Flash
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return HAL_ERROR;
    }

    // 执行扇区擦除
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &sector_error);

    // 重新锁定 Flash
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief  将数据缓冲区写入 FLASH
 * @note   数据按 32 位 Word 写入，地址与长度必须 4 字节对齐
 *         写入后会立即进行数据回读校验
 *
 * @param  start_addr  写入起始物理地址
 * @param  p_data      指向待写入数据的缓冲区
 * @param  data_size   写入数据长度，单位：字节（必须是 4 的倍数）
 *
 * @retval HAL_OK      写入及校验成功
 * @retval HAL_ERROR   参数错误、地址越界、写入失败或校验失败
 */
HAL_StatusTypeDef FlashIf_Write(uint32_t start_addr, const uint8_t *p_data, uint32_t data_size)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t          write_val; // 待写入的一字长值 (4 字节)

    /* 1. 参数检查 */
    if ((p_data == NULL) || (data_size == 0U))
    {
        return HAL_ERROR;
    }

    /* Flash Word 写入要求地址与长度均 4 字节对齐 */
    if (((start_addr & 0x3U) != 0U) || ((data_size & 0x3U) != 0U))
    {
        return HAL_ERROR;
    }

    if (FlashIf_CheckRange(start_addr, data_size) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 2. 解锁 Flash */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 3. 循环写入并校验 */
    for (uint32_t i = 0U; i < data_size; i += 4U)
    {
        /* 利用 memcpy 提取 4 字节，安全规避 p_data 指针未对齐引发的 HardFault */
        memcpy(&write_val, p_data + i, sizeof(uint32_t));

        /* STM32F407 FLASH_TYPEPROGRAM_WORD: 一次写入 32 bit */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, start_addr, write_val) != HAL_OK)
        {
            status = HAL_ERROR;
            break;
        }

        /*
         * 写入后立即回读校验
         * 比较 Flash 中读取的 32 位数据与写入的 write_val 是否一致
         */
        if (*(volatile uint32_t *)start_addr != write_val)
        {
            status = HAL_ERROR;
            break;
        }

        /* 下一个 Word 地址 */
        start_addr += 4U;
    }

    /* 4. 锁定 Flash */
    HAL_FLASH_Lock();

    return status;
}

/**
  * @brief  获取 Flash 指定区域的读写保护状态
  * @param  start_addr 起始物理地址
  * @param  data_size  检查区域大小（字节）
  * @retval 保护状态组合值（Flash_Protection_None / Flash_Protection_WRP / Flash_Protection_RDP）
  */
FlashIf_Protection_t FlashIf_GetProtectionStatus(uint32_t start_addr, uint32_t data_size)
{
    FlashIf_Protection_t       protection_status  = FlashIf_Protection_None;
    FLASH_OBProgramInitTypeDef OptionsBytesStruct = {0};

    // 起始与结束扇区
    uint32_t start_sector, end_sector;

    //扇区掩码
    uint32_t target_sectors_mask = 0U;

    if (FlashIf_CheckRange(start_addr, data_size) != HAL_OK)
    {
        return FlashIf_Protection_Error;
    }

    // 计算起始与结束扇区
    start_sector = FlashIf_GetSector(start_addr);
    end_sector   = FlashIf_GetSector(start_addr + data_size - 1U);

    /* 1. 动态生成传入区域对应的扇区掩码 */
    for (uint32_t sector = start_sector; sector <= end_sector; sector++)
    {
        // STM32F4 的 WRP 掩码第 N 位代表 Sector N
        target_sectors_mask |= (1U << sector);
    }

    /* 2. 解锁 Flash 及 Option Bytes 以允许读取 */
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    /* 3. 获取当前 Option Bytes 寄存器配置 */
    HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);

    /* 4. 读取完毕立刻上锁，缩短安全漏洞窗口 */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    /* 5. 检查读保护 (RDP) 状态 */
    if (OptionsBytesStruct.RDPLevel != OB_RDP_LEVEL_0)
    {
        protection_status |= FlashIf_Protection_RDP;
    }

    /* 6. 动态检查目标区域的写保护 (WRP) 状态 */
    /* 注：F4 硬件逻辑中，WRP 寄存器位为 0 代表开启保护，1 代表无保护 */
    if (((~OptionsBytesStruct.WRPSector) & target_sectors_mask) != 0x00U)
    {
        protection_status |= FlashIf_Protection_WRP;
    }

    return protection_status;
}

/* ==================== Private Implementation ===================== */
/* 💡 这里放：本文件内部 static 辅助函数的具体实现 */

/**
 * @brief  根据 Flash 物理地址获取对应的 STM32F4 扇区号
 * @param  address Flash 地址 (0x08000000 ~ 0x080FFFFF)
 * @return 扇区号 (FLASH_SECTOR_0 ~ FLASH_SECTOR_11)，若地址非法返回 0xFF
 */
static uint32_t FlashIf_GetSector(uint32_t Address)
{
    uint32_t sector;

    if ((Address >= 0x08000000) && (Address < 0x08004000))
    {
        sector = FLASH_SECTOR_0;
    }
    else if ((Address >= 0x08004000) && (Address < 0x08008000))
    {
        sector = FLASH_SECTOR_1;
    }
    else if ((Address >= 0x08008000) && (Address < 0x0800C000))
    {
        sector = FLASH_SECTOR_2;
    }
    else if ((Address >= 0x0800C000) && (Address < 0x08010000))
    {
        sector = FLASH_SECTOR_3;
    }
    else if ((Address >= 0x08010000) && (Address < 0x08020000))
    {
        sector = FLASH_SECTOR_4;
    }
    else if ((Address >= 0x08020000) && (Address < 0x08040000))
    {
        sector = FLASH_SECTOR_5;
    }
    else if ((Address >= 0x08040000) && (Address < 0x08060000))
    {
        sector = FLASH_SECTOR_6;
    }
    else if ((Address >= 0x08060000) && (Address < 0x08080000))
    {
        sector = FLASH_SECTOR_7;
    }
    else if ((Address >= 0x08080000) && (Address < 0x080A0000))
    {
        sector = FLASH_SECTOR_8;
    }
    else if ((Address >= 0x080A0000) && (Address < 0x080C0000))
    {
        sector = FLASH_SECTOR_9;
    }
    else if ((Address >= 0x080C0000) && (Address < 0x080E0000))
    {
        sector = FLASH_SECTOR_10;
    }
    else if ((Address >= 0x080E0000) && (Address < 0x08100000))
    {
        sector = FLASH_SECTOR_11;
    }
    else
    {
        sector = FLASH_IF_INVALID_SECTOR;
    }

    return sector;
}

/**
 * @brief  检查 FLASH 地址范围及扇区有效性
 *
 * @param  start_addr 起始地址
 * @param  data_size  数据大小，单位：字节
 *
 * @retval HAL_OK      地址范围有效
 * @retval HAL_ERROR   参数非法或超出 Flash 范围
 */
static HAL_StatusTypeDef FlashIf_CheckRange(uint32_t start_addr, uint32_t data_size)
{
    uint32_t start_sector;
    uint32_t end_sector;

    /* 数据大小不能为 0 */
    if (data_size == 0U)
    {
        return HAL_ERROR;
    }

    /* 检查 Flash 地址范围 */
    if (start_addr > (FLASH_IF_END_ADDRESS - data_size + 1U))
    {
        return HAL_ERROR;
    }

    start_sector = FlashIf_GetSector(start_addr);
    end_sector   = FlashIf_GetSector(start_addr + data_size - 1U);

    /* 检查扇区有效性 */
    if ((start_sector == FLASH_IF_INVALID_SECTOR) || (end_sector == FLASH_IF_INVALID_SECTOR) ||
        (start_sector > end_sector))
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
