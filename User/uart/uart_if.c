/**
 * @brief   串口模块接口
 */

/* ==================== Included Headers =========================== */
/* 标准 C 库头文件 (C Standard Library) */

/* 芯片/厂商/第三方库头文件 (Platform/OS/HAL Library) */

/* 用户自定义的模块头文件 (User/Application Headers) */

#include "uart_if.h"
#include "buffer_queue.h"
#include "usart.h"
#include "dma.h"

/* ==================== Private Constants & Defines ================ */
/* 💡 这里放：仅限本文件内部使用的宏定义、私有常量 */

//缓冲区个数，需要为2的幂
#define QUEUE_NODE_NUM 2U

#if ((QUEUE_NODE_NUM == 0) || ((QUEUE_NODE_NUM & (QUEUE_NODE_NUM - 1)) != 0))
#error "缓冲区个数，需要为2的幂"
#endif

//每个节点对应的接收缓冲区大小，单位字节
#define QUEUE_NODE_BUF_SIZE 1030U

/* ==================== Private Types ============================== */
/* 💡 这里放：仅限本文件内部使用、不需要暴露给外部的私有结构体或联合体定义 */

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Private Variables ========================== */
/* 💡 这里放：本文件内部使用的静态全局变量 */

//创建一个BOOTLOADER接收队列
static BufferQueue_t bl_queue;

//创建N个节点
static BQ_Node_t bl_queue_nodes[QUEUE_NODE_NUM];

//创建实际的缓冲区，即数据帧实际存放位置
__align(4) static uint8_t bl_queue_bufs[QUEUE_NODE_NUM][QUEUE_NODE_BUF_SIZE];

/* ==================== Private Declarations ======================= */
/* 💡 这里放：本文件内部 static 辅助函数的声明 */

static HAL_StatusTypeDef UartIf_StartReceive(void);

/* ==================== Public APIs ================================ */
/* 💡 这里放：对外公开接口的具体实现 */

/**
  * @brief  正常接收回调：只要总线空闲（IDLE）或者数组收满（TC），系统自动进这里
  * @param  huart: 串口句柄
  * @param  Size:  底层硬件为你精确计算出的“本次实际收到的字节数”
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        // 保存实际接收长度
        BQ_CURR_WR_NODE(&bl_queue)->len = Size;

        //记录总接收字节
        bl_queue.total_rx_bytes += Size;

        //当前帧写入结束
        BQ_WriteFinish(&bl_queue);

        //开启接收
        UartIf_StartReceive();
    }
}

/**
  * @brief  异常错误回调：当串口被高频数据轰炸、或者静电干扰引爆 ORE（过载错误）时，系统进这里收尸
  * @param  huart: 串口句柄
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 1. 确认是不是ORE错误
        if ((huart->ErrorCode & HAL_UART_ERROR_ORE) != 0)
        {
            // 2. 清除硬件的 ORE 错误标志
            __HAL_UART_CLEAR_OREFLAG(huart);

            // 3. 错误码清零
            huart->ErrorCode = HAL_UART_ERROR_NONE;

            //开启接收
            UartIf_StartReceive();
        }
    }
}

/**
  * @brief  初始化
  */
void UartIf_Init(void)
{
    BQ_Init(
        &bl_queue,
        QUEUE_NODE_NUM,
        bl_queue_nodes,
        QUEUE_NODE_BUF_SIZE,
        (uint8_t *)bl_queue_bufs);

    //开启接收
    UartIf_StartReceive();
}

/**
  * @brief  读取一帧数据
  * @param  len: 存放数据长度
  * @retval  uint8_t *:  指向数据缓冲区的指针
  */
uint8_t *UartIf_ReadBlock(uint16_t *len)
{
    //获取可读取的缓冲区指针
    BQ_Node_t *data = BQ_Read(&bl_queue);

    if (data == NULL)
    {
        return NULL;
    }

    if (DMA_IsEnabled(huart1.hdmarx) == false)
    {
        UartIf_StartReceive();
    }

    *len = data->len;

    return data->frame_buf;
}

/**
  * @brief  读取一帧数据结束
  */
void UartIf_ReadBlockFinish(void)
{
    BQ_ReadFinish(&bl_queue);
}

/**
  * @brief  发送一个字节
  */
void UartIf_SendByte(uint8_t byte)
{
    HAL_UART_Transmit(&huart1, &byte, 1, 10);
}

/**
  * @brief  发送字符串
  */
void UartIf_SendData(uint8_t *p_data, uint16_t len)
{
    if (p_data != NULL && len > 0)
    {
        HAL_UART_Transmit(&huart1, p_data, len, 1000);
    }
}

/* ==================== Private Implementation ===================== */
/* 💡 这里放：本文件内部 static 辅助函数的具体实现 */

/**
  * @brief  开启数据接收
  * @param  huart: 串口句柄指针
  * @param  queue:缓冲队列结构体
  * @retval HAL_StatusTypeDef: HAL库状态码
  */
static HAL_StatusTypeDef UartIf_StartReceive(void)
{
    //获取写缓冲区指针，准备写入新数据
    BQ_Node_t *data = BQ_Write(&bl_queue);

    //如果队列已满，则停止接收，不会继续开启DMA
    if (data == NULL)
    {
        bl_queue.overflow_count++;

        return HAL_ERROR;
    }

    // 重新开启串口 DMA 接收
    //HAL_UARTEx_ReceiveToIdle_DMA内部会重新把所有DMA中断打开
    //所以每次重新拉起 DMA 后，必须手动关闭半传输中断（HT）
    HAL_StatusTypeDef res =
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, data->frame_buf, bl_queue.frame_max_len);

    // 启动成功后，手动关闭 DMA 半传输中断（HT），仅保留传输完成（TC）与空闲中断（IDLE）
    if (res == HAL_OK)
    {
        __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    }

    return res;
}
