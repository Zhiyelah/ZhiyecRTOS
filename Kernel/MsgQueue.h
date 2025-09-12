/**
 * @file MsgQueue.h
 * @author Zhiyelah
 * @brief 消息队列
 * @note 可选的模块
 */

#ifndef _MsgQueue_h
#define _MsgQueue_h

#include "StackList.h"
#include "Tick.h"
#include <stdbool.h>
#include <stddef.h>

struct MsgQueue {
    /* 数据缓冲区 */
    void *buffer;
    /* 缓冲区大小 */
    size_t buffer_size;
    /* 类型大小 */
    size_t type_size;
    /* 等待发送消息的任务 */
    struct StackList tasks_waiting_to_send;
    /* 等待接收消息的任务 */
    struct StackList tasks_waiting_to_receive;
    /* 等待接收消息的任务数 */
    size_t task_count;

    /* 队头指针（出队位置） */
    size_t queue_head;
    /* 队尾指针（入队位置） */
    size_t queue_tail;
    /* 当前队列元素数量 */
    volatile size_t queue_count;
};

/**
 * @brief 初始化消息队列
 * @param msg_queue 消息对象
 * @param type_size 消息数据类型大小
 * @param buffer 消息缓冲区
 * @param buffer_size 缓冲区大小
 */
void MsgQueue_init(struct MsgQueue *const msg_queue,
                   const size_t type_size, void *const buffer, const size_t buffer_size);

/**
 * @brief 发送消息
 * @param msg_queue 消息对象
 * @param data 消息内容
 * @return 是否发送成功
 */
bool MsgQueue_send(struct MsgQueue *const msg_queue, const void *const data);

/**
 * @brief 发送消息
 * @param msg_queue 消息对象
 * @param data 消息内容
 * @return 是否发送成功
 * @note 中断安全的版本
 */
bool MsgQueue_sendFromISR(struct MsgQueue *const msg_queue, const void *const data);

/**
 * @brief 接收消息
 * @param msg_queue 消息对象
 * @param data 消息存放变量
 * @return 是否接收成功
 */
void MsgQueue_receive(struct MsgQueue *const msg_queue, void *const data);

/**
 * @brief 尝试接收消息, 超时直接返回失败
 * @param msg_queue 消息对象
 * @param data 消息存放变量
 * @param timeout 超时时间
 * @return 是否接收成功
 */
bool MsgQueue_tryReceive(struct MsgQueue *const msg_queue, void *const data,
                         const Tick_t timeout);

#endif /* _MsgQueue_h */
