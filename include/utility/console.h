/**
 * @file console.h
 * @author Zhiyelah
 * @brief 控制台支持
 * @note 可通过终端模式下的串口工具与MCU交互
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

struct console_cmd {
    const char *name;                       // 命令名称
    const char *description;                // 命令描述
    int (*handler)(int argc, char *argv[]); // 命令处理函数
};

/**
 * @brief 向控制台输入一个字符
 * @param ch 输入的字符
 */
void console_input_char(char ch);

/**
 * @brief 注册控制台命令
 * @param cmd 控制台命令对象
 */
void console_register_cmd(const struct console_cmd *const cmd);

#endif /* _CONSOLE_H */
