#ifndef __BUFFER_QUEUE_H
#define __BUFFER_QUEUE_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ==================== Exported Constants & Defines =============== */
/* 💡 这里放：对外公开的宏定义、常量、配置项 (如 #define QUEUE_SIZE 256) */

//获取当前写节点
#define BQ_CURR_WR_NODE(queue) BQ_IdxToNode((queue), (queue)->curr_write_idx)
//获取当前读节点
#define BQ_CURR_RD_NODE(queue) BQ_IdxToNode((queue), (queue)->curr_read_idx)

/*信息输出*/
#define BUFFER_QUEUE_DEBUG_ENABLE 0

#if (BUFFER_QUEUE_DEBUG_ENABLE == 1)
#define BUFFER_QUEUE_DEBUG(fmt, arg...)                                                            \
    printf("<<-QUEUE-DEBUG->> [%s:%d]" fmt "\n", __FUNCTION__, __LINE__, ##arg);

#else
#define QUEUE_DEBUG(fmt, arg...)                                                                   \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#endif

/* ==================== Exported Types ============================= */
/* 💡 这里放：外部需要用到的结构体定义、枚举定义、typedef、别名等 */

// 声明节点索引类型
typedef uint8_t BQ_NodeIndex_t;

/**
  * @brief  BootLoader数据帧
  * @note   一个节点对应一个缓冲区，一个缓冲区对应一帧数据
  */
typedef struct
{
    uint8_t *frame_buf; // 存放数据帧的缓冲区
    uint16_t len;       // 数据帧实际长度
} BQ_DataFrame_t;

//缓冲区队列的节点数据类型
typedef BQ_DataFrame_t BQ_Node_t;

/**
  * @brief  缓冲区队列
  * @note   设计两套读写指针，是为了区分"正在操作（Current）"和"已经完成（Committed）"的缓冲区
  */
typedef struct
{
    BQ_NodeIndex_t read_idx;       //读指针
    BQ_NodeIndex_t write_idx;      //写指针
    BQ_NodeIndex_t curr_read_idx;  //正在读取的缓冲区指针
    BQ_NodeIndex_t curr_write_idx; //正在写入的缓冲区指针

    BQ_NodeIndex_t node_count;    //队列包含的节点数
    BQ_Node_t     *nodes;         //指向数据帧数组
    uint16_t       frame_max_len; //一帧的最大长度，即缓冲区的大小

    //溢出次数
    //当队列写满后时，新到来的数据将被丢弃，同时该计数加一。
    uint32_t overflow_count;
    uint32_t total_rx_bytes; //累计接收字节数
} BufferQueue_t;

/* ==================== Exported Variables ========================= */
/* 💡 这里放：允许外部访问的全局变量声明 (加 extern，尽量少用) */

/* ==================== Exported APIs ============================== */
/* 💡 这里放：所有对外公开的函数声明 (外部可以调用的核心接口) */

//获取可写入的缓冲区指针
BQ_Node_t *BQ_Write(BufferQueue_t *queue);
//获取可读取的缓冲区指针
BQ_Node_t *BQ_Read(BufferQueue_t *queue);
//一个缓冲区读取完毕
void BQ_ReadFinish(BufferQueue_t *queue);
//一个缓冲区写入完毕
void BQ_WriteFinish(BufferQueue_t *queue);
//创建缓冲队列
bool BQ_Init(
    BufferQueue_t *queue,
    BQ_NodeIndex_t node_count,
    BQ_Node_t     *nodes,
    uint16_t       buf_size,
    uint8_t       *buf);

/**
  * @brief  虚拟指针转实际数组下标 (利用2的幂次掩码)
  */
static inline BQ_NodeIndex_t BQ_GetRealIdx(const BufferQueue_t *queue, BQ_NodeIndex_t idx)
{
    return idx & ((queue->node_count) - 1U);
}

/**
  * @brief  对缓冲队列的指针加1
  * @param  queue:缓冲队列结构体
  * @param  idx：要加1的读写指针
  * @return  返回加1的结果
  */
static inline BQ_NodeIndex_t BQ_IncrIdx(const BufferQueue_t *queue, BQ_NodeIndex_t idx)
{
    //read and write pointers incrementation is done modulo 2*size
    return (idx + 1) & (2 * (queue->node_count) - 1U);
}

/**
  * @brief  虚拟指针直接转节点结构体指针
  */
static inline BQ_Node_t *BQ_IdxToNode(const BufferQueue_t *queue, BQ_NodeIndex_t idx)
{
    return &(queue->nodes[BQ_GetRealIdx(queue, idx)]);
}

/**
  * @brief  判断缓冲队列是(1)否(0)已满
  * @param  queue:缓冲队列
  */
static inline bool BQ_IsFull(const BufferQueue_t *queue)
{
    //翻转 write_idx的二进制最高位，表示写指针已经超过读指针一圈了
    return queue->write_idx == (queue->read_idx ^ queue->node_count);
}

/**
  * @brief  判断缓冲队列是(1)否(0)全空
  * @param  queue:缓冲队列
  */
static inline bool BQ_IsEmpty(const BufferQueue_t *queue)
{
    return queue->write_idx == queue->read_idx;
}

#endif /* __BUFFER_QUEUE_H */
