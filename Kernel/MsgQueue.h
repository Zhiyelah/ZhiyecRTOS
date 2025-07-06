/**
 * @file MsgQueue.h
 * @author Zhiyelah
 * @brief 消息队列
 */

#ifndef _MsgQueue_h
#define _MsgQueue_h

#include <stdbool.h>
#include <stddef.h>

struct MsgQueue {
    /* 数据缓冲区 */
    void *buffer;
    /* 缓冲区大小 */
    size_t buffer_size;
    /* 类型大小 */
    size_t type;

    /* 队头指针（出队位置） */
    size_t head;
    /* 队尾指针（入队位置） */
    size_t tail;
    /* 当前元素数量 */
    size_t count;
};

/**
 * @brief 创建一个消息队列
 * @param msq_queue 消息对象
 * @param type 消息类型
 * @param buffer 消息缓冲区
 * @param buffer_size 缓冲区大小
 */
void MsgQueue_new(struct MsgQueue *const msg_queue,
                  const size_t type, void *const buffer, const size_t buffer_size);

/**
 * @brief 发送消息
 * @param msq_queue 消息对象
 * @param data 消息内容
 * @return 是否发送成功
 */
bool MsgQueue_send(struct MsgQueue *const msg_queue, void *const data);

/**
 * @brief 接收消息
 * @param msq_queue 消息对象
 * @param data 消息存放变量
 * @return 是否接收成功
 */
bool MsgQueue_receive(struct MsgQueue *const msg_queue, void *const data);

#endif /* _MsgQueue_h */
