/**
 * @brief   E2模块接口
 */

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

#include "i2c.h"

/* 用户自定义的模块头文件 (User/Application Headers) */

#include "eeprom.h"

/* ==================== Private Constants & Defines ================ */
/* 💡 这里放：仅限本文件内部使用的宏定义、私有常量 */

// E2的地址
#define E2_I2C_ADDR  0xA0U
//E2的数据最大/结束地址
#define E2_END_ADDR  0xFFU
// E2 单页大小（AT24C02 为 8 字节）
#define E2_PAGE_SIZE 8U

/* ==================== Private Types ============================== */
/* 💡 这里放：仅限本文件内部使用、不需要暴露给外部的私有结构体或联合体定义 */

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Private Variables ========================== */
/* 💡 这里放：本文件内部使用的静态全局变量 */

/* ==================== Private Declarations ======================= */
/* 💡 这里放：本文件内部 static 辅助函数的声明 */

/* ==================== Public APIs ================================ */
/* 💡 这里放：对外公开接口的具体实现 */

/**
 * @brief      从E2中读取数据
 *
 * @details    detailed description
 *
 * @param      data_addr:E2中的数据地址
 *
 * @param      p_data:存放读取数据的缓冲区
 *
 * @param      size:需要读取的字节数
 *
 * @return     true: 读取成功 | false: 读取失败
 */
bool E2_ReadData(uint8_t data_addr, const uint8_t *p_data, uint16_t size)
{
    // 空指针及 0 长度校验
    if ((p_data == NULL) || (size == 0U))
    {
        return false;
    }

    // 边界检查：确保 (起始地址 + 读取长度) 不超过芯片最大容量
    if (((uint32_t)data_addr + size - 1U) > E2_END_ADDR)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &hi2c1,
        E2_I2C_ADDR,
        data_addr,
        I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)p_data,
        size,
        1000);

    if (status != HAL_OK)
    {
        return false;
    }

    return true;
}

/**
 * @brief      向E2写入数据
 *
 * @details    24C02最多连续写入8字节，提交写入后需要等待5ms
 *
 * @param      data_addr:E2中的数据地址
 *
 * @param      p_data:存放读取数据的缓冲区
 *
 * @param      size:需要写入的字节数
 *
 * @return     true: 写入成功 | false: 写入失败
 */
bool E2_WriteData(uint8_t data_addr, const uint8_t *p_data, uint16_t size)
{
    // 1. 参数及边界校验
    if ((p_data == NULL) || (size == 0))
    {
        return false;
    }

    if (((uint32_t)data_addr + size - 1) > E2_END_ADDR)
    {
        return false;
    }

    // 2. 类型保持与 size 一致，防止隐式截断
    uint16_t write_len = 0;

    // 3. 跨页拆分写入
    while (size > 0)
    {
        // 使用位与运算高效计算当前页剩余可写字节数 (替代 % 运算)
        write_len = E2_PAGE_SIZE - (data_addr & (E2_PAGE_SIZE - 1U));

        if (size < write_len)
        {
            write_len = size;
        }

        // 发送单页数据
        if (HAL_I2C_Mem_Write(
                &hi2c1,
                E2_I2C_ADDR,
                data_addr,
                I2C_MEMADD_SIZE_8BIT,
                (uint8_t *)p_data,
                write_len,
                1000) != HAL_OK)
        {
            return false;
        }

        // ACK Polling：芯片内部烧写完成后会自动 ACK (重试 10 次，每次超时 1ms)
        if (HAL_I2C_IsDeviceReady(&hi2c1, E2_I2C_ADDR, 10, 1) != HAL_OK)
        {
            return false; // 超时未响应，可能硬件故障
        }

        // 维护偏移量和剩余长度
        size      -= write_len;
        data_addr += (uint8_t)write_len;
        p_data    += write_len;
    }

    return true;
}

/* ==================== Private Implementation ===================== */
/* 💡 这里放：本文件内部 static 辅助函数的具体实现 */
