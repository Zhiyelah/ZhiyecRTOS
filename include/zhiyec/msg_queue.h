/**
 * @file msg_queue.h
 * @author Zhiyelah
 * @brief 消息队列
 * @note 可选的模块
 */

#ifndef _ZHIYEC_MSGQUEUE_H
#define _ZHIYEC_MSGQUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <zhiyec/tick.h>

struct msgqueue;

#define MSGQUEUE_BYTE 36

/**
 * @brief 初始化消息队列
 * @param msg_queue_mem 对象内存指针
 * @param type_size 消息数据类型大小
 * @param buffer 消息缓冲区
 * @param buffer_size 缓冲区大小
 * @return 对象指针
 */
struct msgqueue *msgqueue_init(void *const msg_queue_mem,
                               const size_t type_size, void *const buffer, const size_t buffer_size);

/**
 * @brief 发送消息
 * @param msg_queue 消息对象
 * @param data 消息内容
 * @return 是否发送成功
 */
bool msgqueue_send(struct msgqueue *const msg_queue, const void *const data);

/**
 * @brief 发送消息
 * @param msg_queue 消息对象
 * @param data 消息内容
 * @return 是否发送成功
 * @note 中断安全的版本
 */
bool msgqueue_send_from_isr(struct msgqueue *const msg_queue, const void *const data);

/**
 * @brief 接收消息
 * @param msg_queue 消息对象
 * @param data 消息存放变量
 * @return 是否接收成功
 */
void msgqueue_receive(struct msgqueue *const msg_queue, void *const data);

/**
 * @brief 尝试接收消息, 超时直接返回失败
 * @param msg_queue 消息对象
 * @param data 消息存放变量
 * @param timeout 超时时间
 * @return 是否接收成功
 */
bool msgqueue_try_receive(struct msgqueue *const msg_queue, void *const data,
                          const tick_t timeout);

#endif /* _ZHIYEC_MSGQUEUE_H */
