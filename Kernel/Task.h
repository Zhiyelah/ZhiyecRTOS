/**
 * @file Task.h
 * @author Zhiyelah
 * @brief 任务调度
 */

#ifndef _Task_h
#define _Task_h

#include "Config.h"
#include "Port.h"
#include "Tick.h"
#include <stdbool.h>
#include <stdint.h>

/* 32位系统的栈单位大小 */
typedef uint32_t Stack_t;

enum TaskType {
    /* 普通任务 */
    COMMON_TASK = 0U,
    /* 实时任务 */
    REALTIME_TASK
};

enum SchedMethod {
    /* 时间片轮转调度 */
    SCHED_RR = 0U,
    /* 先进先出调度 */
    SCHED_FIFO
};

struct TaskAttribute {
    /**
     * 任务类型, 可以是下列的其中一个:
     *      COMMON_TASK,
     *      REALTIME_TASK
     */
    const enum TaskType type;

    /**
     * 调度方法, 仅对实时任务有效, 可以是下列的其中一个:
     *      SCHED_RR,
     *      SCHED_FIFO
     */
    const enum SchedMethod sched_method;

    /* 堆栈指针(在开启动态内存分配时可选择为空) */
    Stack_t *const stack;

    /* 堆栈大小 */
    const Stack_t stack_size;
};

/**
 * @brief 任务属性结构体定义
 * @param _name 结构体变量名
 * @param _stack 任务栈指针
 * @param _type 任务类型
 */
#define TaskAttribute_def(_name, _stack, _type)           \
    struct TaskAttribute _name = {                        \
        .stack = _stack,                                  \
        .stack_size = sizeof(_stack) / sizeof(_stack[0]), \
        .type = _type,                                    \
    }

struct TaskStruct {
    /* 栈顶指针(必须是结构体的第一个成员) */
    volatile Stack_t *top_of_stack;
    /* 堆栈指针 */
    Stack_t *stack;
#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    /* 是否为动态分配栈 */
    bool is_dynamic_stack;
#endif
    /* 任务类型 */
    enum TaskType type;
    /* 调度方法 */
    enum SchedMethod sched_method;
    /* 挂起恢复的时间 */
    Tick_t resume_time;
};

/**
 * @brief 创建任务 线程安全
 * @param fn 任务函数
 * @param arg 任务函数参数
 * @param attr 任务属性
 */
bool Task_create(void (*const fn)(void *), void *const arg, const struct TaskAttribute *attr);

/**
 * @brief 开始执行任务, 不会执行到该函数下文
 */
int Task_exec(void);

/**
 * @brief 当前任务进入阻塞, 并切换到下一个任务, 一段时间后恢复
 * @param ticks 阻塞时间
 */
void Task_sleep(const Tick_t ticks);

/**
 * @brief 在任务中调用, 删除当前任务
 */
void Task_deleteSelf(void);

/**
 * @brief 让出CPU
 */
#define yield()                              \
    {                                        \
        Interrupt_CTRL_Reg = PendSV_SET_Bit; \
        __dsb(0xFu);                         \
        __isb(0xFu);                         \
    }

/**
 * @brief 判断当前环境是否在任务里
 * @return 如果当前在任务里则返回true, 否则返回false
 */
#define Task_isInTask() Port_isUsingPSP()

/**
 * @brief 暂停调度
 */
void Task_suspendScheduling(void);

/**
 * @brief 恢复调度
 */
void Task_resumeScheduling(void);

#endif /* _Task_h */
