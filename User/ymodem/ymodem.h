/**
 * @brief   YModem模块头文件
 */

#ifndef __YMODEM_H
#define __YMODEM_H

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

#include <stdint.h>

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

/* 用户自定义的模块头文件 (User/Application Headers) */

/* ==================== Exported Constants & Defines =============== */
/* 💡 这里放：对外公开的宏定义、常量、配置项 (如 #define QUEUE_SIZE 256) */

/* ==================== Exported Types ============================= */
/* 💡 这里放：外部需要用到的结构体定义、枚举定义、typedef、别名等 */

/**
 * @brief  YModem 接收结果
 */
typedef enum
{
    YModemResult_OK = 0, /* 接收成功 */
    YModemResult_Error,  /* 接收失败 */

} YModemResult_t;

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Exported APIs ============================== */
/* 💡 这里放：所有对外公开的函数声明 (外部可以调用的核心接口) */

YModemResult_t YModem_Receive(void);

#endif /* __YMODEM_H */
