/**
 * @brief   串口模块接口头文件
 */

#ifndef __UART_IF_H
#define __UART_IF_H

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

#include <stdint.h>

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

/* 用户自定义的模块头文件 (User/Application Headers) */

/* ==================== Exported Constants & Defines =============== */
/* 💡 这里放：对外公开的宏定义、常量、配置项 (如 #define QUEUE_SIZE 256) */

/* ==================== Exported Types ============================= */
/* 💡 这里放：外部需要用到的结构体定义、枚举定义、typedef、别名等 */

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Exported APIs ============================== */
/* 💡 这里放：所有对外公开的函数声明 (外部可以调用的核心接口) */

//初始化
void UartIf_Init(void);
//读取一帧数据
uint8_t *UartIf_ReadBlock(uint16_t *len);
//读取一帧数据结束
void UartIf_ReadBlockFinish(void);
//发送一个字节
void UartIf_SendByte(uint8_t byte);
//发送字符串
void UartIf_SendData(uint8_t *p_data, uint16_t len);

#endif /* __UART_IF_H */
