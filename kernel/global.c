#include <zhiyec/task.h>
#include <zhiyec/tick.h>

/**
 * @brief 内核Tick
 */
volatile tick_t kernel_ticks = 0U;

/**
 * @brief 中断禁用嵌套计数
 */
volatile int interrupt_disabled_nesting = 0;

/**
 * @brief 任务列表索引
 * @param index 任务优先级
 */
struct QueueList kernel_task_list[TASKPRIORITY_NUM];

/**
 * @brief 任务列表位图
 */
uint32_t kernel_task_list_bitmap = 0U;
