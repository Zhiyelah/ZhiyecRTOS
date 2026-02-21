/**
 * @file task.h
 * @author Zhiyelah
 * @brief 任务调度
 */

#ifndef _ZHIYEC_TASK_H
#define _ZHIYEC_TASK_H

#include <../kernel/port.h>
#include <config.h>
#include <stdbool.h>
#include <stdint.h>
#include <zhiyec/compiler.h>
#include <zhiyec/kernel.h>
#include <zhiyec/list.h>
#include <zhiyec/types.h>

/* 内存字节对齐位 */
#define BYTE_ALIGNMENT 8

enum TaskPriority {
    TASKPRIORITY_IDLE = 0U,
    TASKPRIORITY_LOW,
    TASKPRIORITY_MEDIUM,
    TASKPRIORITY_HIGH,

    /* 枚举数量(必须是枚举的最后一位) */
    TASKPRIORITY_NUM
};

enum SchedMethod {
    /* 时间片轮转调度 */
    SCHED_RR = 0U,
    /* 先进先出调度 */
    SCHED_FIFO
};

struct TaskAttribute {
    /* 任务优先级 */
    enum TaskPriority priority;
    /* 调度方法 */
    enum SchedMethod sched_method;
    /* 任务销毁回调函数 */
    void (*destroy)(stack_t *);
};

struct TaskStruct {
    /* 栈顶指针(必须是结构体的第一个成员) */
    volatile stack_t *top_of_stack;
    /* 堆栈指针 */
    stack_t *stack;
    /* 任务属性 */
    struct TaskAttribute attr;
    /* 任务链表 */
    struct SListHead task_node;
    /* 恢复执行的时间 */
    tick_t resume_time;
};

/**
 * @brief 创建任务
 * @param fn 任务函数
 * @param arg 任务函数参数
 * @param stack 任务栈指针
 * @param stack_size 任务栈大小(单位: 字(word))
 * @param attr 任务属性
 * @note 线程安全
 */
bool Task_create(void (*const fn)(void *), void *const arg, stack_t *const stack, const stack_t stack_size,
                 const struct TaskAttribute *const attr);

/**
 * @brief 开始调度任务, 不会执行到该函数下文
 */
int Task_schedule(void);

/**
 * @brief 任务睡眠
 * @param ticks 阻塞时间
 * @note 当前任务进入阻塞, 并切换到下一个任务, 一段时间后恢复
 */
void Task_sleep(const tick_t ticks);

/**
 * @brief 绝对时间睡眠
 * @param prev_wake_time 上次唤醒时间
 * @param interval 时间间隔
 * @note 当前任务进入阻塞, 直到指定的时间点; 如果超时立即返回, 确保任务执行的周期性
 */
void Task_sleepUntil(tick_t *const prev_wake_time, const tick_t interval);

/**
 * @brief 在任务中调用, 删除当前任务
 */
void Task_deleteLater(void);

/**
 * @brief 暂停所有任务
 */
void Task_suspendAll(void);

/**
 * @brief 恢复所有任务
 */
void Task_resumeAll(void);

extern struct TaskStruct *volatile kernel_current_task;

/**
 * @brief 获取当前任务
 * @return 当前任务指针
 */
#define Task_currentTask() (kernel_current_task)

/**
 * @brief 获取任务的优先级
 * @param task 任务指针
 * @return 任务优先级
 */
static always_inline enum TaskPriority Task_getPriority(const struct TaskStruct *const task) {
    return task->attr.priority;
}

/**
 * @brief 设置任务的优先级
 * @param task 任务指针
 * @param priority 任务优先级
 */
static always_inline void Task_setPriority(struct TaskStruct *const task, const enum TaskPriority priority) {
    task->attr.priority = priority;
}

/**
 * @brief 获取节点所在任务
 * @param node 任务节点
 * @return 节点所在任务
 */
static always_inline struct TaskStruct *Task_fromTaskNode(const struct SListHead *const node) {
    return container_of(node, struct TaskStruct, task_node);
}

/**
 * @brief 让出CPU
 */
#define Task_yield() Port_yield()

#endif /* _ZHIYEC_TASK_H */
