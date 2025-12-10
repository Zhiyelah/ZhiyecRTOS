/**
 * @file console.h
 * @author Zhiyelah
 * @brief 控制台支持
 * @note 可通过终端模式下的串口工具与MCU交互
 */

#ifndef _Console_h
#define _Console_h

struct ConsoleCommand {
    const char *name;                       // 命令名称
    const char *description;                // 命令描述
    int (*handler)(int argc, char *argv[]); // 命令处理函数
};

/**
 * @brief 控制台主循环, 用于处理输入
 * @param ch 输入的字符
 */
void Console_process(char ch);

/**
 * @brief 注册控制台命令
 * @param cmd 控制台命令对象
 */
void Console_registerCommand(const struct ConsoleCommand *const cmd);

#endif /* _Console_h */
