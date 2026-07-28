/**
 * @brief   YMODEM模块
 */

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

/* 用户自定义的模块头文件 (User/Application Headers) */

#include "ymodem.h"
#include "uart/uart_if.h"
#include "flash/flash_if.h"

/* ==================== Private Constants & Defines ================ */
/* 💡 这里放：仅限本文件内部使用的宏定义、私有常量 */

// clang-format off
//用于CRC16计算
static const uint16_t crc16_table[256] =
{
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
    0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,

    0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,
    0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,

    0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
    0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,

    0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,
    0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,

    0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,
    0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,

    0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,
    0xDBFD,0xCBDC,0xFBBF,0xEB9E,0x9B79,0x8B58,0xBB3B,0xAB1A,

    0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,
    0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,

    0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
    0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,

    0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,
    0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,

    0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,
    0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,

    0xB5EA,0xA5CB,0x95A8,0x8589,0xF56E,0xE54F,0xD52C,0xC50D,
    0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,

    0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,
    0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,

    0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
    0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x3882,0x28A3,

    0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,
    0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,

    0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,
    0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,

    0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,
    0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};
// clang-format on

/* ========================================================================== */
/*                       YModem 协议自定义参数                                */
/* ========================================================================== */

//允许接收的文件名最大长度
#define YMODEM_FILE_NAME_MAX_LEN 64U
//允许接收的文件最大体积
#define YMODEM_FILE_MAX_SIZE     FLASH_IF_APP_MAX_SIZE
//错误计数溢出值
#define YMODEM_MAX_ERRORS        10U

/* ========================================================================== */
/*                       YModem 协议帧数据格式                                  */
/* ========================================================================== */

/* 包头固定长度: SOH/STX(1B) + 包号(1B) + 包号反码(1B) */
#define YMODEM_HEAD_SIZE    3U
/* 包尾固定长度: 16位 CRC 校验和(2B) 高字节在前，低字节在后 */
#define YMODEM_TRAILER_SIZE 2U

/* 数组下标严格定义（从 0 开始） */
#define YMODEM_IDX_START    0U /* SOH 或者 STX 的下标 */
#define YMODEM_IDX_NUM      1U /* 当前包号的下标 */
#define YMODEM_IDX_NUM_COMP 2U /* 包号反码的下标 */
#define YMODEM_IDX_DATA     3U /* 纯数据净荷的起始下标 */

/* 数据包有效载荷大小 */
#define YMODEM_PAYLOAD_SIZE_SOH 128U
#define YMODEM_PAYLOAD_SIZE_STX 1024U

/* ========================================================================== */
/*                       YModem 协议标准控制字符                      */
/* ========================================================================== */
#define YMODEM_CTRL_SOH 0x01U          /* 128字节 小包起始符 */
#define YMODEM_CTRL_STX 0x02U          /* 1024字节 大包起始符 */
#define YMODEM_CTRL_EOT 0x04U          /* 传输结束标志 */
#define YMODEM_CTRL_ACK 0x06U          /* 正确应答 */
#define YMODEM_CTRL_NAK 0x15U          /* 错误应答 */
#define YMODEM_CTRL_CAN 0x18U          /* 中止符 */
#define YMODEM_CTRL_C   ((uint8_t)'C') /* 字符 'C'，请求 16-bit CRC */

/* 古老的超级终端键盘强行中止快捷键 ('A' 和 'a')，若不用可直接删掉 */
#define YMODEM_ABORT_KEY_UPPER ((uint8_t)'A')
#define YMODEM_ABORT_KEY_LOWER ((uint8_t)'a')

/* ==================== Private Types ============================== */
/* 💡 这里放：仅限本文件内部使用、不需要暴露给外部的私有结构体或联合体定义 */

/**
  * @brief  YModem状态机
  */
typedef enum
{
    /* 空闲 */
    YModemState_Idle = 0,

    /* 等待文件信息块 (Block 0) */
    YModemState_WaitBlock0,

    /* 等待文件数据块 (Block 1 ~ N) */
    YModemState_WaitDataBlock,

    /* 收到第一次 EOT，等待第二次 EOT */
    YModemState_WaitSecondEot,

    /* 双 EOT 完成，等待最后空 Block 0 */
    YModemState_WaitEndBlock

} YModemState_t;

/**
  * @brief  YModem 协议控制与状态管理核心结构体
  */
typedef struct
{
    /* 当前状态 */
    YModemState_t state; /* 当前工作状态 (显式枚举类型，便于调测) */

    /* 状态超时管理 */
    uint32_t state_enter_tick;
    uint32_t state_timeout_ms;
    uint8_t  state_timeout_count; //超时次数统计

    /* 错误统计 */
    uint8_t bad_block_count; //坏包统计

    /* 块管理 */
    uint8_t last_block_number; /* 上一个成功处理的块序号，用于时序校验 */

    /* 文件信息 */
    char     file_name[YMODEM_FILE_NAME_MAX_LEN + 1U]; /* 固件文件名 (预留 '\0' 结束符空间) */
    uint32_t file_size;           /* 固件总文件大小 (全局固定不变，用于校验与进度) */
    uint32_t remaining_file_size; /* 剩余待接收的文件大小 (随数据写入实时递减) */
    uint32_t file_write_addr;     /* 记录当前应写入的FLASH地址 */

} YModem_t;

/**
  * @brief  YModem Block 0 元数据解析结果
  */
typedef enum
{
    YModemBlock0Result_OK = 0,

    /* 文件信息错误 */
    YModemBlock0Result_FileNameEmpty,
    YModemBlock0Result_FileSizeEmpty,
    YModemBlock0Result_FileSizeOver,
    YModemBlock0Result_FileSizeNotDecimal,

    /* 包序号错误 */
    YModemBlock0Result_WrongNumber,

} YModemBlock0Result_t;

/**
  * @brief  YModem DataBlock 元数据解析结果
  */
typedef enum
{
    YModemDataBlockResult_OK = 0,

    /* 包序号重复 */
    YModemDataBlockResult_RepeatedNumber,

    /* 包序号错误 */
    YModemDataBlockResult_WrongNumber,

} YModemDataBlockResult_t;

/**
  * @brief  YModem EndBlock 元数据解析结果
  */
typedef enum
{
    YModemEndBlockResult_OK = 0,

    YModemEndBlockResult_Error,

} YModemEndBlockResult_t;

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Private Variables ========================== */
/* 💡 这里放：本文件内部使用的静态全局变量 */

static YModem_t s_ymodem;

/* ==================== Private Declarations ======================= */
/* 💡 这里放：本文件内部 static 辅助函数的声明 */

static void YModem_Init(void);

static YModemResult_t YModem_ProcessData(uint8_t *p_data);

static YModemResult_t YModem_HandleBlock(uint8_t *p_data);
static void YModem_HandleEot(void);

static void YModem_SetState(YModemState_t state);

static bool YModem_ValidatePacket(const uint8_t *data);
static YModemBlock0Result_t YModem_ParseBlock0(const uint8_t *p_data);
static YModemDataBlockResult_t YModem_ParseDataBlock(const uint8_t *p_data);
static YModemResult_t YModem_WriteDataToFlash(uint8_t *p_data);
static YModemEndBlockResult_t YModem_ParseEndBlock(const uint8_t *p_data);

static void YModem_LogBlock0Error(YModemBlock0Result_t status);
static inline void YModem_LogFinalResult(void);

static YModemResult_t YModem_CheckTimeout(void);
static YModemResult_t YModem_HandleTimeout(void);

static inline void YModem_Abort(void);

/* ==================== Public APIs ================================ */
/* 💡 这里放：对外公开接口的具体实现 */

/**
  * @brief  计算CRC16
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t CRC16_Calculate(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0x0000;

    while (length--)
    {
        uint8_t index = (uint8_t)((crc >> 8) ^ *data++);
        crc           = (crc << 8) ^ crc16_table[index];
    }

    return crc;
}

/**
  * @brief  使用 YModem-CRC16 协议接收固件并同步烧录进 Flash
  * @param  file_max_size 允许接收的文件最大体积
  * @retval YModemResult_t 接收结果状态
            YModemResult_OK 表示成功，YModemResult_Error为失败
  */
YModemResult_t YModem_Receive(void)
{
    uint8_t *data;      //数据缓冲区
    uint16_t data_size; //数据帧长度

    YModemResult_t res;

    printf("\r\n========已进入等待接收状态，30秒未收到自动退出========\r\n");

    /* 1. 初始化状态机 */
    YModem_Init();

    /* 2. 主接收轮询循环 */
    while (s_ymodem.state != YModemState_Idle)
    {
        //获取一包数据
        data = UartIf_ReadBlock(&data_size);

        if (data == NULL)
        {
            /* 3. 超时检测 */
            if (YModem_CheckTimeout() == YModemResult_Error)
            {
                return YModemResult_Error;
            }
            continue;
        }

        //处理数据
        res = YModem_ProcessData(data);

        UartIf_ReadBlockFinish(); //读取结束，释放缓冲区

        if (res == YModemResult_Error)
        {
            return YModemResult_Error;
        }
    }

    YModem_LogFinalResult();

    return YModemResult_OK;
}

/* ==================== Private Implementation ===================== */
/* 💡 这里放：本文件内部 static 辅助函数的具体实现 */

/**
  * @brief  初始化 YModem 接收状态机
  */
static void YModem_Init(void)
{

    /* 结构体全局清零 */
    memset(&s_ymodem, 0, sizeof(s_ymodem));

    s_ymodem.file_write_addr = FLASH_IF_APP_START_ADDRESS;

    /* 切入 WaitBlock0 状态，自动配置 3 秒闹钟并刷新 tick */
    YModem_SetState(YModemState_WaitBlock0);

    /* 启动接收：向上位机发送首个 'C' 字符请求传输 */
    UartIf_SendByte(YMODEM_CTRL_C);
}

/**
  * @brief  处理YModem 收到的数据包
  * @param  p_data: 数据缓冲区地址
  * @retval YModemResult_t执行结果
  */
static YModemResult_t YModem_ProcessData(uint8_t *p_data)
{
    char ctrl_char = p_data[0];

    YModemResult_t res;

    switch (ctrl_char)
    {
        case YMODEM_CTRL_SOH:
        case YMODEM_CTRL_STX:
            /* 先进行数据校验 */
            if (YModem_ValidatePacket(p_data))
            {
                res = YModem_HandleBlock(p_data);
                if (res == YModemResult_Error)
                {
                    return YModemResult_Error;
                }
            }
            else
            {
                UartIf_SendByte(YMODEM_CTRL_NAK);
                s_ymodem.bad_block_count++;
            }
            break;

        case YMODEM_CTRL_EOT:
            /* EOT处理 */
            YModem_HandleEot();
            break;

        case YMODEM_CTRL_CAN:
            /* 任意状态下：收到双 CAN 直接重置状态机退出 */
            if (p_data[1] == YMODEM_CTRL_CAN)
            {
                YModem_SetState(YModemState_Idle);
                return YModemResult_Error;
            }
            else
            {
                s_ymodem.bad_block_count++;
            }

            break;

        case YMODEM_ABORT_KEY_UPPER:
        case YMODEM_ABORT_KEY_LOWER:
            printf("用户取消发送 \r\n");

            /* 用户按键强制取消发送，终止接收*/
            YModem_Abort();

            return YModemResult_Error;

        default:
            s_ymodem.bad_block_count++;
            break;
    }

    /* 🛡️ 安全保护：连续 10 次坏包/错包，终止接收 */
    if (s_ymodem.bad_block_count >= YMODEM_MAX_ERRORS)
    {
        YModem_Abort();
        return YModemResult_Error;
    }

    return YModemResult_OK;
}

/**
  * @brief  处理 SOH(128B) / STX(1024B) 数据块接收事件
  * @param  p_data: 串口接收队列节点指针
  * @retval YModemResult_t执行结果
  */
static YModemResult_t YModem_HandleBlock(uint8_t *p_data)
{
    switch (s_ymodem.state)
    {
        /* ------------------- 1. 等待 Block 0 (元数据包) ------------------- */
        case YModemState_WaitBlock0:
        {
            YModemBlock0Result_t res = YModem_ParseBlock0(p_data);

            if (res == YModemBlock0Result_OK)
            {
                /* 成功获取文件名/大小：回 ACK 确认，再发 'C' 催促上位机发送第一个数据包 */
                UartIf_SendByte(YMODEM_CTRL_ACK);
                UartIf_SendByte(YMODEM_CTRL_C);

                s_ymodem.bad_block_count   = 0;
                s_ymodem.last_block_number = 0;
                YModem_SetState(YModemState_WaitDataBlock);

                if (FlashIf_GetProtectionStatus(FLASH_IF_APP_START_ADDRESS, s_ymodem.file_size) !=
                    FlashIf_Protection_None)
                {
                    YModem_Abort();
                    YModem_SetState(YModemState_Idle);

                    printf("\r\n FLASH禁止写入 \r\n");
                    return YModemResult_Error;
                }

                if (FlashIf_Erase(FLASH_IF_APP_START_ADDRESS, s_ymodem.file_size) != HAL_OK)
                {

                    YModem_Abort();
                    YModem_SetState(YModemState_Idle);

                    printf("\r\n FLASH擦除失败 \r\n");
                    return YModemResult_Error;
                }
            }
            else if (res == YModemBlock0Result_WrongNumber)
            {
                /* 非 Block 0：回 NAK 要求重传 */
                UartIf_SendByte(YMODEM_CTRL_NAK);
                s_ymodem.bad_block_count++;
            }
            else
            {
                YModem_LogBlock0Error(res);

                /* 文件名空、超大等严重错误：终止接收 */
                YModem_Abort();

                return YModemResult_Error;
            }
            break;
        }

        /* ------------------- 2. 等待传输真正的 Firmware 数据包 ------------------- */
        case YModemState_WaitDataBlock:
        {
            YModemDataBlockResult_t res = YModem_ParseDataBlock(p_data);

            if (res == YModemDataBlockResult_OK)
            {
                /* 收到正确数据包：回 ACK，直接用物理帧的包号同步（兼容 255->0/1 翻转） */
                UartIf_SendByte(YMODEM_CTRL_ACK);

                s_ymodem.bad_block_count   = 0;
                s_ymodem.last_block_number = p_data[YMODEM_IDX_NUM];

                if (YModem_WriteDataToFlash(p_data) != YModemResult_OK)
                {
                    YModem_Abort();
                    YModem_SetState(YModemState_Idle);

                    printf("\r\n FLASH写入失败 \r\n");
                    return YModemResult_Error;
                }
            }
            else if (res == YModemDataBlockResult_RepeatedNumber)
            {
                /*
                 * 重复包：
                 * 通常由 ACK 丢失导致，补发 ACK 保持同步。
                 * 若连续出现大量重复包，说明通信链路异常，
                 * 累计异常次数用于最终退出保护。
                 */
                UartIf_SendByte(YMODEM_CTRL_ACK);
                s_ymodem.bad_block_count++;
            }
            else
            {
                /* 包号乱序：回 NAK 提醒上位机重传 */
                UartIf_SendByte(YMODEM_CTRL_NAK);
                s_ymodem.bad_block_count++;
            }
            break;
        }

        /* ------------------- 3. 等待全 0 结束包 (全流程收尾) ------------------- */
        case YModemState_WaitEndBlock:
        {
            YModemEndBlockResult_t res = YModem_ParseEndBlock(p_data);

            if (res == YModemEndBlockResult_OK)
            {
                /* 全 0 包确认完毕：回 ACK，整个升级流程完美结束 */
                UartIf_SendByte(YMODEM_CTRL_ACK);
                YModem_SetState(YModemState_Idle);
            }
            else
            {
                UartIf_SendByte(YMODEM_CTRL_NAK);
                s_ymodem.bad_block_count++;
            }
            break;
        }

        default:
            s_ymodem.bad_block_count++;
            break;
    }

    return YModemResult_OK;
}

/**
  * @brief  处理 EOT 传输结束请求
  */
static void YModem_HandleEot(void)
{
    switch (s_ymodem.state)
    {
        case YModemState_WaitDataBlock:
            /* 第一次收到 EOT，回 NAK */
            UartIf_SendByte(YMODEM_CTRL_NAK);

            YModem_SetState(YModemState_WaitSecondEot);

            break;

        case YModemState_WaitSecondEot:
            /* 第二次收到 EOT，回 ACK 和 C 催促其发送全 0 结束包 */
            UartIf_SendByte(YMODEM_CTRL_ACK);
            UartIf_SendByte(YMODEM_CTRL_C);

            YModem_SetState(YModemState_WaitEndBlock);

            break;

        default:
            s_ymodem.bad_block_count++;
            break;
    }
}

/**
  * @brief  统一状态切换函数
  * @param  state 目标切换状态
  */
static void YModem_SetState(YModemState_t state)
{
    /* 1. 改变当前状态 */
    s_ymodem.state = state;

    /* 2. 记录进入该状态的黄金时间戳 */
    s_ymodem.state_enter_tick = HAL_GetTick();

    /* 4. 根据新状态，集中配置专属的超时闹钟 */
    switch (state)
    {
        case YModemState_WaitBlock0:
            s_ymodem.state_timeout_ms = 3000U; /* 等待元数据包，限时 3 秒 */
            break;

        case YModemState_WaitDataBlock:
        case YModemState_WaitSecondEot:
            s_ymodem.state_timeout_ms = 1000U; /* 数据传输与 EOT 握手，限时 1 秒 */
            break;

        case YModemState_WaitEndBlock:
            s_ymodem.state_timeout_ms = 2000U; /* 等待全 0 结束包，限时 2 秒 */
            break;

        case YModemState_Idle:
        default:
            s_ymodem.state_timeout_ms = 0U; /* 退出/空闲状态，关闭超时检测闹钟 */
            break;
    }
}

/**
  * @brief  校验 YModem 数据包静态合法性 (仅校验数据结构与内容是否损坏)
  * @param  data: 接收到的整包数据缓冲区起始指针 (包含包头和包尾)
  * @retval true:  数据包完整且无损坏
  *         false: 数据包损坏 (包号反码不对、或 CRC 校验失败)
  */
static bool YModem_ValidatePacket(const uint8_t *data)
{
    uint16_t crc_target;
    uint16_t crc_cal;
    uint16_t data_len;

    /* 1. 检查包头起始符并确定数据载荷长度 */
    if (data[YMODEM_IDX_START] == YMODEM_CTRL_SOH)
    {
        data_len = YMODEM_PAYLOAD_SIZE_SOH;
    }
    else
    {
        data_len = YMODEM_PAYLOAD_SIZE_STX;
    }

    /* 2. 检查包号与其反码是否严格匹配 */
    if ((data[YMODEM_IDX_NUM] ^ data[YMODEM_IDX_NUM_COMP]) != 0xFF)
    {
        return false; /* 静态校验失败：包号取反损坏 */
    }

    /* 3. 提取包尾的 2 字节目标 CRC */
    uint32_t crc_index = YMODEM_HEAD_SIZE + data_len;
    crc_target         = (uint16_t)((uint16_t)data[crc_index] << 8);
    crc_target        |= data[crc_index + 1];

    /* 4. 计算纯数据净荷的 CRC16 校验值 */
    crc_cal = CRC16_Calculate(&data[YMODEM_IDX_DATA], data_len);

    /* 5. 比对 CRC 暗号 */
    if (crc_cal != crc_target)
    {
        return false; /* 静态校验失败：数据传输有误码 */
    }

    /* 静态校验完全通过 */
    return true;
}

/**
  * @brief  第0包元数据解析
  * @param  p_data: 接收到的整包数据缓冲区起始指针
            已通过 YModem_ValidatePacket() 校验的数据包
            包含完整包头、数据区和CRC
  * @retval 解析状态结果
  */
static YModemBlock0Result_t YModem_ParseBlock0(const uint8_t *p_data)
{
    uint32_t i         = YMODEM_IDX_DATA;
    uint32_t j         = 0;
    uint32_t file_size = 0;

    //判断是否为第0包
    if (p_data[YMODEM_IDX_NUM] != 0U)
    {
        return YModemBlock0Result_WrongNumber;
    }

    /* 1. 提取固件文件名 */
    while (isprint((uint8_t)p_data[i]) && (j < YMODEM_FILE_NAME_MAX_LEN))
    {
        s_ymodem.file_name[j] = (char)p_data[i];
        i++;
        j++;
    }
    s_ymodem.file_name[j] = '\0';

    if (j == 0)
    {
        return YModemBlock0Result_FileNameEmpty;
    }

    /* 2. 越过文件名与大小字符串之间的 '\0' 分隔符 */
    while (p_data[i] != '\0')
    {
        i++;
    }
    i++;

    /* 3. 前置拦截：跳过文件名后如果立刻就是 '\0'，说明大小字段为空 */
    if (p_data[i] == '\0')
    {
        return YModemBlock0Result_FileSizeEmpty;
    }

    /* 4. 提取并实时计算固件大小 */
    while (isdigit((uint8_t)p_data[i]))
    {
        /*  拦截乘法溢出 */
        if (file_size > (UINT32_MAX / 10U))
        {
            return YModemBlock0Result_FileSizeOver;
        }

        file_size = file_size * 10U + (uint32_t)(p_data[i] - '0');
        i++;

        /* 实时拦截超出芯片预设最大空间的固件 */
        if (file_size > FLASH_IF_APP_MAX_SIZE)
        {
            return YModemBlock0Result_FileSizeOver;
        }
    }

    // 存入文件大小
    s_ymodem.file_size           = file_size;
    s_ymodem.remaining_file_size = file_size;
    /* 5. 格式完备性校验：数字提取停下来后，必须直接以 '\0' 或者空格结尾 */
    if ((p_data[i] != '\0') && ((p_data[i] != ' ')))
    {
        /* 如果停下来后 data[i] 还是数字，说明是前面防乘法溢出拦截导致 i 没有自增 */
        if (isdigit((uint8_t)p_data[i]))
        {
            return YModemBlock0Result_FileSizeOver;
        }
        else
        {
            return YModemBlock0Result_FileSizeNotDecimal;
        }
    }

    return YModemBlock0Result_OK;
}

/**
  * @brief  实际有效的数据包解析
  * @param  p_data: 接收到的整包数据缓冲区起始指针
            已通过 YModem_ValidatePacket() 校验的数据包
            包含完整包头、数据区和CRC
  * @retval 解析状态结果
  */
static YModemDataBlockResult_t YModem_ParseDataBlock(const uint8_t *p_data)
{
    uint8_t packet_number   = p_data[YMODEM_IDX_NUM];
    uint8_t expected_number = (uint8_t)(s_ymodem.last_block_number + 1U);

    /* 1. 兼容 255 的下一个包是 1 的情况 (1-based 循环) */
    if ((s_ymodem.last_block_number == 255U) && (packet_number == 1U))
    {
        return YModemDataBlockResult_OK;
    }
    /* 2. 当前包号 == 上一包包号 + 1 (自然递增或 255->0 自然溢出) */
    else if (packet_number == expected_number)
    {
        return YModemDataBlockResult_OK;
    }
    /* 3. 上位机没收到 ACK 导致的重传重复包 */
    else if (packet_number == s_ymodem.last_block_number)
    {
        return YModemDataBlockResult_RepeatedNumber;
    }
    /* 4. 其它乱序或跳包错误 */
    else
    {
        return YModemDataBlockResult_WrongNumber;
    }
}

/**
 * @brief  将 YMODEM 当前数据块写入 Flash
 * @return YModemDataBlockResult_t 写入结果
 */
static YModemResult_t YModem_WriteDataToFlash(uint8_t *p_data)
{
    uint32_t data_len = YMODEM_PAYLOAD_SIZE_STX;
    uint32_t flash_write_len;

    /* 1. 如果是最后一包，实际有效数据不足一个整包 */
    if (data_len > s_ymodem.remaining_file_size)
    {
        data_len = s_ymodem.remaining_file_size;
    }

    flash_write_len = data_len;

    if ((flash_write_len & 3) != 0)
    {
        uint32_t pad = 4U - (flash_write_len & 3U);

        memset(p_data + YMODEM_IDX_DATA + flash_write_len, 0xFF, pad);

        flash_write_len += pad;
    }

    /* 4. 执行 Flash 写入 */
    HAL_StatusTypeDef status =
        FlashIf_Write(s_ymodem.file_write_addr, p_data + YMODEM_IDX_DATA, flash_write_len);

    /* 5. 写入成功后统一更新地址与剩余长度 */
    if (status != HAL_OK)
    {
        return YModemResult_Error;
    }

    s_ymodem.file_write_addr     += flash_write_len;
    s_ymodem.remaining_file_size -= data_len;

    return YModemResult_OK;
}

/**
  * @brief  全0包元数据解析
  * @param  p_data: 接收到的整包数据缓冲区起始指针
            已通过 YModem_ValidatePacket() 校验的数据包
            包含完整包头、数据区和CRC
  * @retval 解析状态结果
  */
static YModemEndBlockResult_t YModem_ParseEndBlock(const uint8_t *p_data)
{
    uint16_t i;

    if (p_data[YMODEM_IDX_NUM] != 0U)
    {
        return YModemEndBlockResult_Error;
    }

    /* 检查数据区是否全0 */
    for (i = 0U; i < YMODEM_PAYLOAD_SIZE_SOH; i++)
    {
        if (p_data[YMODEM_IDX_DATA + i] != 0U)
        {
            return YModemEndBlockResult_Error;
        }
    }

    return YModemEndBlockResult_OK;
}

/**
  * @brief  Block0错误打印调试
  * @param  status: YModemBlock0Result_t枚举值
  */
static void YModem_LogBlock0Error(YModemBlock0Result_t status)
{
    switch (status)
    {
        case YModemBlock0Result_FileNameEmpty:
            printf("文件名为空\r\n");
            break;

        case YModemBlock0Result_FileSizeEmpty:
            printf("文件大小为0\r\n");
            break;

        case YModemBlock0Result_FileSizeOver:
            printf("文件过大\r\n");
            break;

        case YModemBlock0Result_FileSizeNotDecimal:
            printf("文件大小格式错误\r\n");
            break;

        default:
            break;
    }
}

/**
  * @brief  接收成功后打印结果
  * @param  status: YModemBlock0Result_t枚举值
  */
static inline void YModem_LogFinalResult(void)
{
    printf("\r\n ========正在打印文件信息======== \r\n");
    printf("\r\n ========文件名：%s======== \r\n", s_ymodem.file_name);
    printf("\r\n ========文件大小：%d======== \r\n", s_ymodem.file_size);
}

/**
  * @brief  接收状态超时检查
  * @retval YModemResult_t执行结果
  */
static YModemResult_t YModem_CheckTimeout(void)
{
    uint32_t tick = HAL_GetTick();

    /* 如果当前状态没开启超时管理（时限为 0），直接返回 */
    if (s_ymodem.state_timeout_ms == 0U)
    {
        return YModemResult_OK;
    }

    /* 当前滴答减去进入状态的起点 */
    if ((tick - s_ymodem.state_enter_tick) < s_ymodem.state_timeout_ms)
    {
        return YModemResult_OK;
    }

    s_ymodem.state_timeout_count++;

    /* 🔔 刷新进入状态的时间基准 */
    s_ymodem.state_enter_tick = tick;

    /* 判断超时计数是否溢出 */
    if (s_ymodem.state_timeout_count >= YMODEM_MAX_ERRORS)
    {
        YModem_Abort();
        return YModemResult_Error;
    }

    /* 触发超时惩罚 */
    return YModem_HandleTimeout();
}

/**
  * @brief  接收状态超时事件处理
  * @retval YModemResult_t执行结果
  */
static YModemResult_t YModem_HandleTimeout(void)
{
    switch (s_ymodem.state)
    {
        case YModemState_WaitBlock0:
            UartIf_SendByte(YMODEM_CTRL_C);
            break;

        case YModemState_WaitDataBlock:
        case YModemState_WaitSecondEot:
        case YModemState_WaitEndBlock:
            UartIf_SendByte(YMODEM_CTRL_NAK);
            break;

        default:
            /* 未知状态，不应该继续通信 */
            YModem_Abort();

            return YModemResult_Error;
    }
    return YModemResult_OK;
}

/**
 * @brief  主动终止 YModem 接收流程
 */
static inline void YModem_Abort(void)
{
    /* 发送双 CAN，通知发送端终止传输 */
    UartIf_SendByte(YMODEM_CTRL_CAN);
    UartIf_SendByte(YMODEM_CTRL_CAN);

    /* 切换为空闲状态 */
    YModem_SetState(YModemState_Idle);
}
