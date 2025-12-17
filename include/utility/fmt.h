/**
 * @file fmt.h
 * @author Zhiyelah
 * @brief 格式化输入输出
 */

#ifndef _PRINTF_H
#define _PRINTF_H

struct PrintStream {
    void (*write)(char);
};

/**
 * @brief 设置输出
 */
void fmt_setOut(struct PrintStream *print_stream);

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

#endif /* _PRINTF_H */
