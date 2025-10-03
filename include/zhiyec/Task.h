/**
 * @file Task.h
 * @author Zhiyelah
 * @brief 任务调度
 */

#ifndef _Task_h
#define _Task_h

#include <../kernel/Port.h>
#include <Config.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zhiyec/List.h>
#include <zhiyec/Tick.h>
#include <zhiyec/Types.h>

enum TaskType {
    /* 普通任务 */
    COMMON_TASK = 0U,
    /* 实时任务 */
    REALTIME_TASK,

    /* 枚举数量(必须是枚举的最后一位) */
    TASKTYPE_NUM
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
    enum TaskType type;

    /**
     * 调度方法, 仅对实时任务有效, 可以是下列的其中一个:
     *      SCHED_RR,
     *      SCHED_FIFO
     */
    enum SchedMethod sched_method;

    /* 堆栈指针(在开启动态内存分配时可选择为空) */
    stack_t *stack;

    /* 堆栈大小 */
    stack_t stack_size;
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

struct TaskStruct;

/**
 * @brief 创建任务
 * @param fn 任务函数
 * @param arg 任务函数参数
 * @param attr 任务属性
 * @note 线程安全
 */
bool Task_create(void (*const fn)(void *), void *const arg, const struct TaskAttribute *attr);

/**
 * @brief 开始执行任务, 不会执行到该函数下文
 */
int Task_exec(void);

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
void Task_deleteSelf(void);

/**
 * @brief 获取任务个数
 * @return 任务个数
 */
size_t Task_getCount(void);

/**
 * @brief 暂停所有任务
 */
void Task_suspendAll(void);

/**
 * @brief 恢复所有任务
 */
void Task_resumeAll(void);

/**
 * @brief 是否需要切换任务
 * @return true表示需要切换任务, 否则不需要
 */
bool Task_needSwitch(void);

/**
 * @brief 获取当前任务
 * @return 当前任务指针
 */
struct TaskStruct *Task_currentTask(void);

/**
 * @brief 获取任务的类型
 * @param task 任务指针
 * @return 任务类型
 */
enum TaskType Task_getType(const struct TaskStruct *const task);

/**
 * @brief 设置任务的类型
 * @param task 任务指针
 * @param type 任务类型
 */
void Task_setType(struct TaskStruct *const task, const enum TaskType type);

/**
 * @brief 获取节点所在任务
 * @param node 任务节点
 * @return 节点所在任务
 */
struct TaskStruct *Task_fromTaskNode(const struct SListHead *const node);

/**
 * @brief 让出CPU
 */
#define Task_yield()                         \
    do {                                     \
        Interrupt_CTRL_Reg = PendSV_SET_Bit; \
        __dsb(0xFu);                         \
        __isb(0xFu);                         \
    } while (0)

/**
 * @brief 开始原子操作
 */
#define Task_beginAtomic() Port_disableInterrupt()

/**
 * @brief 结束原子操作
 */
#define Task_endAtomic() Port_enableInterrupt()

/**
 * @brief 原子操作块
 */
#define Task_atomic(code_block)        \
    do {                               \
        Task_beginAtomic();            \
        {code_block} Task_endAtomic(); \
    } while (0)

#endif /* _Task_h */
