#ifndef _Console_h
#define _Console_h

#include "Printf.h"

/* 控制台配置参数 */
#define INPUT_BUFFER_SIZE 128    // 输入缓冲区大小
#define COMMAND_MAX_ARGS 8       // 最大参数数量
#define PROMPT_STRING "zhiyec> " // 命令提示符

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
