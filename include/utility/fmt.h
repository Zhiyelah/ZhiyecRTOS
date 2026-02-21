/**
 * @file fmt.h
 * @author Zhiyelah
 * @brief 格式化输入输出
 */

#ifndef _ZHIYEC_FMT_H
#define _ZHIYEC_FMT_H

#include <zhiyec/task.h>

struct PrintStream {
    void (*write)(char);
};

/**
 * @brief 初始化fmt
 * @param print_stream 打印流
 * @param print_lock_ceiling_priority 打印锁的天花板优先级
 */
void fmt_init(struct PrintStream *print_stream, const enum TaskPriority print_lock_ceiling_priority);

/**
 * @brief 打印
 */
void fmt_print(const char *str);

/**
 * @brief 打印并换行
 */
void fmt_println(const char *str);

/**
 * @brief 格式化打印
 */
void fmt_printf(const char *format, ...);

#endif /* _ZHIYEC_FMT_H */
