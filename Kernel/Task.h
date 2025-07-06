/**
 * @file Task.h
 * @author Zhiyelah
 * @brief 时间片轮转任务调度
 */

#ifndef _Task_h
#define _Task_h

#include "Port.h"
#include "Tick.h"
#include <stdbool.h>
#include <stdint.h>

/* 32位系统的栈单位大小 */
typedef uint32_t Stack_t;

#define yield() {                           \
    Interrupt_CTRL_Reg = PendSV_SET_Bit;    \
    __dsb(0xFu);                            \
    __isb(0xFu);                            \
}

struct TaskStruct {
    /* 栈顶指针(必须是结构体的第一个成员) */
    volatile Stack_t *top_of_stack;
    /* 堆栈指针 */
    Stack_t *stack;
    /* 状态更新的时间 */
    Tick_t time;
    union {
        /* 已注册的事件 */
        struct EventGroup *event_group;
        /* 等待的消息队列 */
        struct MsgQueue *msg_queue;
    } suspension_reason;
};

/**
 * @brief 创建任务 线程安全
 * @param fn 任务函数
 * @param arg 任务函数参数
 * @param stack 堆栈指针
 * @param stack_size 堆栈大小
 */
bool Task_create(void (*const fn)(void *), void *const arg,
                 Stack_t *const stack, const Stack_t stack_size);

/**
 * @brief 开始执行任务, 不会执行到该函数下文
 */
int Task_exec(void);

/**
 * @brief 当前任务进入阻塞, 并切换到下一个任务, 一段时间后恢复
 * @param ticks 阻塞时间
 */
void Task_delay(const Tick_t ticks);

/**
 * @brief 暂停调度
 */
void Task_suspendScheduling(void);

/**
 * @brief 恢复调度
 */
void Task_resumeScheduling(void);

#endif /* _Task_h */
