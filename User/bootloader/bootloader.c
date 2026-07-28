/**
 * @brief   BootLoader程序
 */

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

#include <string.h>
#include <stdio.h>

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

/* 用户自定义的模块头文件 (User/Application Headers) */

#include "bootloader.h"
#include "ymodem/ymodem.h"
#include "uart/uart_if.h"
#include "flash/flash_if.h"
#include "usart.h"
#include "led/led.h"

/* ==================== Private Constants & Defines ================ */
/* 💡 这里放：仅限本文件内部使用的宏定义、私有常量 */

//注意F407芯片映射在地址 0x2000 0000 的内存有两块，这里是总和
#define CHIP_SRAM_START_ADDR 0x20000000U
#define CHIP_SRAM_END_ADDR   0x2001FFFFU

//BOOT键长按时间
#define BOOT_KEY_LONG_TIME_MS 2000U

/* ==================== Private Types ============================== */
/* 💡 这里放：仅限本文件内部使用、不需要暴露给外部的私有结构体或联合体定义 */

typedef void (*pFunction)(void);

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Private Variables ========================== */
/* 💡 这里放：本文件内部使用的静态全局变量 */

/* ==================== Private Declarations ======================= */
/* 💡 这里放：本文件内部 static 辅助函数的声明 */

static inline void BL_JumpToAppErr(void);

/* ==================== Public APIs ================================ */
/* 💡 这里放：对外公开接口的具体实现 */

/**
 * @brief BootLoader 主菜单
 */
void BL_MainMenu(void)
{
    uint8_t *cmd;       //数据接收缓冲区
    uint16_t cmd_len;   //数据长度
    uint8_t  cmd_char1; //收到的第1个字符
    uint8_t  cmd_char2; //收到的第2个字符

    UartIf_Init();
    FlashIf_Init();

    printf("\r\n ========BootLoader开始运行======== \r\n");

    while (1)
    {
        /* 1. 打印交互菜单 */
        printf("\r\n==============主菜单==============\r\n");
        printf("\r\n 输入数字执行相应操作 \r\n\r\n");
        printf("1. 下载程序到芯片 \r\n");
        printf("2. 跳转到APP \r\n");
        printf("\r\n============================\r\n");

        /* 2. 循环轮询，直到获取到非空的串口数据包 */
        do
        {
            cmd = UartIf_ReadBlock(&cmd_len);
        } while (cmd == NULL);

        cmd_char1 = cmd[0];
        cmd_char2 = cmd[1];

        UartIf_ReadBlockFinish(); //读取结束，释放缓冲区

        /* 3. 校验多字节输入：若第 2 个字符既非回车也非换行，视为非法输入 */
        if (cmd_len > 1)
        {
            if ((cmd_char2 != '\r') && (cmd_char2 != '\n'))
            {
                printf("\r\n 无效命令：输入字符过多 \r\n");
                continue;
            }
        }

        /* 4. 解析首字符命令并执行 */
        switch (cmd_char1)
        {
            case '1': // YMODEM 固件升级
                if (YModem_Receive() == YModemResult_OK)
                {
                    printf("\r\n ========接收成功======== \r\n");
                    printf("\r\n========正在跳转APP========\r\n");
                    BL_JumpToApp();
                }
                else
                {
                    printf("\r\n ========接收失败======== \r\n");
                }
                break;

            case '2': // 直接跳转 APP
                BL_JumpToApp();
                break;

            default: // 未知指令处理
                printf("\r\n 无效的数字: %c，数字只能为1、2 \r\n", cmd_char1);
                break;
        }
    }
}

/**
 * @brief  跳转到用户程序
 */
void BL_JumpToApp(void)
{
    uint32_t app_stack_ptr;
    uint32_t app_reset_handle;

    app_stack_ptr    = *(volatile uint32_t *)FLASH_IF_APP_START_ADDRESS;
    app_reset_handle = *(volatile uint32_t *)(FLASH_IF_APP_START_ADDRESS + 4U);

    /* printf("\r\n========正在跳转APP========\r\n"); */

    /* 1. 检查 MSP 合法性 */
    if ((app_stack_ptr < CHIP_SRAM_START_ADDR) || (app_stack_ptr > CHIP_SRAM_END_ADDR))
    {
        /* printf("APP Stack Error\r\n"); */

        BL_JumpToAppErr();
    }

    /* 2. 检查 Reset_Handler 合法性 (必须在 Flash 范围且 Thumb 态最低位为 1) */
    if ((app_reset_handle < FLASH_IF_APP_START_ADDRESS) ||
        (app_reset_handle > FLASH_IF_END_ADDRESS) || ((app_reset_handle & 0x1U) == 0U))
    {
        /* printf("APP Reset Error\r\n"); */
        
        BL_JumpToAppErr();
    }

    /* 3. 禁止全局中断 */
    __disable_irq();

    /* 4. 关闭 SysTick 滴答定时器 */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 5. 关闭串口以及关联的DMA */
    HAL_UART_DeInit(&huart1);

    /* 6. HAL/RCC 状态恢复 */
    HAL_RCC_DeInit();
    HAL_DeInit();

    /* 7. 清理 NVIC 中断使能与挂起标志 */
    for (uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* 8. 设置 APP 向量表基地址 */
    SCB->VTOR = FLASH_IF_APP_START_ADDRESS;

    /* 9. 设置 APP 主堆栈指针 (MSP) */
    __set_MSP(app_stack_ptr);

    /* 10. 内存与指令屏障：确保寄存器配置彻底生效并刷新指令流水线 */
    __DSB();
    __ISB();

    /* 11. 真正跳转到 APP 的 Reset_Handler */
    pFunction JumpToApp = (pFunction)app_reset_handle;

    JumpToApp();
}

/**
 * @brief      判断BOOT键是否长按
 *
 * @return     true: 按下 | false: 未按下
 */
bool BL_IsBootKeyLongPressed(void)
{

    int count = BOOT_KEY_LONG_TIME_MS / 10;

    // 轮询检测 2000ms
    for (int i = 0; i < count; i++)
    {
        if (HAL_GPIO_ReadPin(BOOT_KEY_GPIO_Port, BOOT_KEY_Pin) != GPIO_PIN_SET)
        {
            return false; // 中途松开，直接判定未按下
        }
        HAL_Delay(10);
    }

    return true; // 连续按住了 2 秒
}

/* ==================== Private Implementation ===================== */
/* 💡 这里放：本文件内部 static 辅助函数的具体实现 */

/**
 * @brief      跳转APP失败后，红灯闪烁提示
 */
static inline void BL_JumpToAppErr(void)
{
        while (1)
        {
            LEDR_TOGGLE;
            HAL_Delay(200);
        }
}
