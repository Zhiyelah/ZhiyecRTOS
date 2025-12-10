#include <zhiyec/Task.h>
#include <zhiyec/Tick.h>

volatile tick_t kernel_ticks = 0U;

/**
 * @brief 中断禁用嵌套计数
 */
volatile int interrupt_disabled_nesting = 0;

/**
 * @brief 任务列表索引
 * @param index 任务类型
 */
struct QueueList kernel_task_list[TASKTYPE_NUM];
