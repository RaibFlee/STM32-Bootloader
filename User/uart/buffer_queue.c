#include "buffer_queue.h"

/* ==================== Private Constants & Defines ================ */
/* 💡 这里放：仅限本文件内部使用的宏定义、私有常量 */

/* ==================== Private Types ============================== */
/* 💡 这里放：仅限本文件内部使用、不需要暴露给外部的私有结构体或联合体定义 */

/* ==================== Private Variables ========================== */
/* 💡 这里放：本文件内部使用的静态全局变量 */

/* ==================== Private Declarations ======================= */
/* 💡 这里放：本文件内部 static 辅助函数的声明 */

static void
BQ_StructInit(BufferQueue_t *queue, BQ_NodeIndex_t node_count, BQ_Node_t *nodes, uint16_t buf_size);
static void
BQ_NodeInit(BQ_NodeIndex_t node_count, BQ_Node_t *nodes, uint16_t buf_size, uint8_t *buf);

#if (BUFFER_QUEUE_DEBUG_ENABLE == 1)

static void BQ_PrintQueue(const BufferQueue_t *queue);
static void BQ_PushTestData(BufferQueue_t *queue, const char *str);
static void BQ_PopTestData(BufferQueue_t *queue);

#endif

/* ==================== Public APIs ================================ */
/* 💡 这里放：对外公开接口的具体实现 */

/**
  * @brief  创建缓冲队列
  * @param  queue:缓冲队列结构体
  * @param  node_count:节点数量
  * @param  nodes:节点数组
  * @param  buf_size:缓冲区大小
  * @param  buf:缓冲区基地址
 * @retval  创建结果false:失败；ture:成功
  */
bool BQ_Init(
    BufferQueue_t *queue,
    BQ_NodeIndex_t node_count,
    BQ_Node_t     *nodes,
    uint16_t       buf_size,
    uint8_t       *buf)
{
    if (queue == NULL || nodes == NULL || buf == NULL)
    {
        return false;
    }

    if ((node_count == 0) || ((node_count & (node_count - 1)) != 0))
    {
        return false; // 节点数不是 2 的幂次，直接拦截
    }

    if (buf_size < 1)
    {
        return false;
    }

    BQ_StructInit(queue, node_count, nodes, buf_size);
    BQ_NodeInit(node_count, nodes, buf_size, buf);

    return true;
}

/**
  * @brief  获取可写入的缓冲区指针
  * @param  queue:缓冲队列结构体
  * @return  可进行写入的缓冲区指针
  * @note  得到指针后可进入写入操作，但写指针不会立即加1，
           写完数据时，应调用QueueWriteFinish对写指针加1
  */
BQ_Node_t *BQ_Write(BufferQueue_t *queue)
{

    if (BQ_IsFull(queue)) /* full, overwrite moves read pointer */
    {
        return NULL;
    }

    return BQ_IdxToNode(queue, queue->curr_write_idx);
}

/**
  * @brief  一个缓冲区写入完毕
  * @param  queue:缓冲队列结构体
  * @note   一帧数据写入完毕，对应的缓冲区就不可以再写入，表示当前缓冲区已满，更新写指针
  */
void BQ_WriteFinish(BufferQueue_t *queue)
{
    //当前节点写完，指向下一个节点
    queue->curr_write_idx = BQ_IncrIdx(queue, queue->curr_write_idx);
    //更新写节点，表示前一个节点写入完成
    queue->write_idx = queue->curr_write_idx;
}

/**
  * @brief  获取可读取的缓冲区指针
  * @param  queue:缓冲队列结构体
  * @return 可进行读取的缓冲区指针
  * @note   得到指针后可进入读取操作，但读指针不会立即加1，
			读取完数据时，应调用QueueReadFinish对读指针加1
  */
BQ_Node_t *BQ_Read(BufferQueue_t *queue)
{
    if (BQ_IsEmpty(queue))
    {
        return NULL;
    }

    return BQ_IdxToNode(queue, queue->curr_read_idx);
}

/**
  * @brief 一个缓冲区读取完毕，释放缓冲区
  * @param  queue:缓冲队列结构体
  * @note   一帧数据读取完毕，对应的缓冲区就不可以再读取，表示当前缓冲区已空，更新读指针
  */
void BQ_ReadFinish(BufferQueue_t *queue)
{
    // 1. 清空已读完节点的长度
    BQ_IdxToNode(queue, queue->curr_read_idx)->len = 0;

    // 当前节点读完，指向下一个节点
    queue->curr_read_idx = BQ_IncrIdx(queue, queue->curr_read_idx);

    //更新读节点，表示前一个节点读取完成
    queue->read_idx = queue->curr_read_idx;
}

/* ==================== Private Implementation ===================== */
/* 💡 这里放：本文件内部 static 辅助函数的具体实现 */

/**
  * @brief  初始化缓冲队列
  * @param  queue:缓冲队列结构体
  * @param  node_count:节点数量
  * @param  nodes:节点数组
  * @param  buf_size:缓冲区大小
  */
static void
BQ_StructInit(BufferQueue_t *queue, BQ_NodeIndex_t node_count, BQ_Node_t *nodes, uint16_t buf_size)
{
    queue->read_idx       = 0;
    queue->write_idx      = 0;
    queue->curr_read_idx  = 0;
    queue->curr_write_idx = 0;

    queue->node_count = node_count;
    queue->nodes      = nodes;

    queue->total_rx_bytes = 0;
    queue->overflow_count = 0;
    queue->frame_max_len  = buf_size;
}

/**
  * @brief  初始化队列节点
  * @param  node_count:节点数量
  * @param  nodes:节点数组
  * @param  buf_size:缓冲区大小
  * @param  buf:缓冲区基地址
  */
static void
BQ_NodeInit(BQ_NodeIndex_t node_count, BQ_Node_t *nodes, uint16_t buf_size, uint8_t *buf)
{
    BQ_NodeIndex_t i;

    for (i = 0; i < node_count; i++)
    {
        nodes[i].frame_buf = buf + i * buf_size; //给节点分配缓冲区
        nodes[i].len       = 0;
    }
}

#if (BUFFER_QUEUE_DEBUG_ENABLE == 1)

// 1. 打印队列状态
static void BQ_PrintQueue(const BufferQueue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }

    BUFFER_QUEUE_DEBUG(
        "read_idx=%d, write_idx=%d, curr_read_idx=%d, curr_write_idx=%d \n",
        queue->read_idx,
        queue->write_idx,
        queue->curr_read_idx,
        queue->curr_write_idx);

    BUFFER_QUEUE_DEBUG(
        "node_count=%d, nodes=%p, overflow_count=%d, total_rx_bytes=%d \n",
        queue->node_count,
        (void *)(queue->nodes),
        queue->overflow_count,
        queue->total_rx_bytes);
}

// 2. 推送测试数据入队
static void BQ_PushTestData(BufferQueue_t *queue, const char *test_str)
{
    if (queue == NULL || test_str == NULL)
    {
        return;
    }

    BQ_Node_t *frame_ptr = BQ_Write(queue);
    if (frame_ptr == NULL)
    {
        return;
    }

    uint16_t str_len = (uint16_t)(strlen(test_str) + 1);

    if (str_len > queue->frame_max_len)
    {
        return;
    }

    frame_ptr->len = str_len;

    for (uint16_t i = 0; i < str_len; i++)
    {
        frame_ptr->frame_buf[i] = test_str[i];
    }

    BQ_WriteFinish(queue);
}

// 3. 从队列拉取数据测试
static void BQ_PopTestData(BufferQueue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }

    BQ_Node_t *frame_ptr = BQ_Read(queue);
    if (frame_ptr == NULL)
    {
        return;
    }

    printf("%s\n", (char *)frame_ptr->frame_buf); // 注意这里应该打印缓冲区的数组
    BQ_ReadFinish(queue);
}
#endif
