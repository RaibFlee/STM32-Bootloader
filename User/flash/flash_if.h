/**
 * @brief   FLASH接口头文件
 */

#ifndef __FLASH_IF_H
#define __FLASH_IF_H

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */
#include <stdint.h>

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

#include "stm32f4xx_hal.h"

/* 用户自定义的模块头文件 (User/Application Headers) */

/* ==================== Exported Constants & Defines =============== */
/* 💡 这里放：对外公开的宏定义、常量、配置项 (如 #define QUEUE_SIZE 256) */

/* Flash 地址范围 */
#define FLASH_IF_START_ADDRESS 0x08000000U
#define FLASH_IF_END_ADDRESS   0x080FFFFFU

/* Application 区域 */
#define FLASH_IF_APP_START_ADDRESS 0x08008000U

/* Application 最大大小 */
#define FLASH_IF_APP_MAX_SIZE (FLASH_IF_END_ADDRESS - FLASH_IF_APP_START_ADDRESS + 1U)

/* ==================== Exported Types ============================= */
/* 💡 这里放：外部需要用到的结构体定义、枚举定义、typedef、别名等 */

/**
 * @brief Flash 保护类型
 */
typedef enum
{
    FlashIf_Protection_None = 0U,

    FlashIf_Protection_WRP = (1U << 0),
    FlashIf_Protection_RDP = (1U << 1),

    //获取过程中发生错误
    FlashIf_Protection_Error = 0xFFU,

} FlashIf_Protection_t;

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Exported APIs ============================== */
/* 💡 这里放：所有对外公开的函数声明 (外部可以调用的核心接口) */

void FlashIf_Init(void);
HAL_StatusTypeDef FlashIf_Erase(uint32_t start_addr, uint32_t data_size);
HAL_StatusTypeDef FlashIf_Write(uint32_t start_addr, const uint8_t *p_data, uint32_t data_size);
FlashIf_Protection_t FlashIf_GetProtectionStatus(uint32_t start_addr, uint32_t data_size);

#endif /* __FLASH_IF_H */
