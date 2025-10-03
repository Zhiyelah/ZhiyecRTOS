/**
 * @file MsgQueue.h
 * @author Zhiyelah
 * @brief 消息队列
 * @note 可选的模块
 */

#ifndef _MsgQueue_h
#define _MsgQueue_h

#include <stdbool.h>
#include <stddef.h>
#include <zhiyec/Tick.h>

struct MsgQueue;

#define MsgQueue_byte 36

/**
 * @brief 初始化消息队列
 * @param msg_queue_mem 对象内存指针
 * @param type_size 消息数据类型大小
 * @param buffer 消息缓冲区
 * @param buffer_size 缓冲区大小
 * @return 对象指针
 */
struct MsgQueue *MsgQueue_init(void *const msg_queue_mem,
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
                         const tick_t timeout);

#endif /* _MsgQueue_h */
