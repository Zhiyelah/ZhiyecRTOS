#include "Console.h"
#include <stdint.h>
#include <string.h>

/* 控制台配置参数 */
#define COMMAND_LIST_SIZE 10     // 已注册命令列表大小
#define INPUT_BUFFER_SIZE 128    // 输入缓冲区大小
#define COMMAND_MAX_ARGS 8       // 最大参数数量
#define PROMPT_STRING "zhiyec> " // 命令提示符

#include "Printf.h"
/* 控制台输出 */
#define Console_printf(...) Printf(__VA_ARGS__)

/* 输入缓存区 */
static char input_buffer[INPUT_BUFFER_SIZE];
static size_t input_len = 0U;

static int console_builtin_cmd_help_handler(int argc, char *argv[]);
static int console_builtin_cmd_version_handler(int argc, char *argv[]);

static const struct ConsoleCommand console_builtin_cmd_help = {
    .name = "help",
    .description = "Show available commands",
    .handler = console_builtin_cmd_help_handler,
};

static const struct ConsoleCommand console_builtin_cmd_version = {
    .name = "version",
    .description = "Show console version",
    .handler = console_builtin_cmd_version_handler,
};

/* 已注册命令列表 */
static const struct ConsoleCommand *commands[COMMAND_LIST_SIZE] = {
    &console_builtin_cmd_help,
    &console_builtin_cmd_version,
};
static size_t command_count = 2U;

/* 处理输入缓冲区中的命令 */
static void Console_handleInputBuffer(void) {
    int argc = 0;
    char *argv[COMMAND_MAX_ARGS];

    /* 解析命令 */
    for (char *token = strtok(input_buffer, " \t\r\n");
         token != NULL && argc < COMMAND_MAX_ARGS;
         token = strtok(NULL, " \t\r\n"), ++argc) {
        argv[argc] = token;
    }

    /* 命令为空 */
    if (argc == 0) {
        return;
    }

    /* 查找并执行对应的命令 */
    for (uint8_t i = 0; i < command_count; i++) {
        if (strcmp(argv[0], commands[i]->name) == 0) {
            commands[i]->handler(argc, argv);
            return;
        }
    }

    Console_printf("Unknown command: %s\r\n", argv[0]);
    Console_printf("Type 'help' for command list\r\n");
}

/* 处理输入 */
void Console_process(char ch) {
    if (ch == '\r') { /* 回车 */
        /* 回显换行 */
        Console_printf("\r\n");

        /* 处理输入缓存区 */
        input_buffer[input_len] = '\0';
        Console_handleInputBuffer();

        /* 清空输入缓存区 */
        input_len = 0;
        memset(input_buffer, 0, INPUT_BUFFER_SIZE);

        /* 打印命令提示符 */
        Console_printf(PROMPT_STRING);

    } else if (ch == '\b' && input_len > 0) { /* 退格 */
        /* 回退缓存区 */
        input_buffer[--input_len] = '\0';

        /* 清除显示的字符 */
        Console_printf("\b \b");

    } else if (input_len < INPUT_BUFFER_SIZE - 1) { /* 普通字符 */
        /* 回显输入字符 */
        Console_printf("%c", ch);

        /* 加入缓存区 */
        input_buffer[input_len++] = ch;
    }
}

/* 注册命令 */
void Console_registerCommand(const struct ConsoleCommand *const cmd) {
    if (command_count < COMMAND_LIST_SIZE) {
        commands[command_count++] = cmd;
    } else {
        Console_printf("Command list full, cannot register %s\r\n", cmd->name);
    }
}

/* 内置命令实现 */

/* 帮助命令 */
static int console_builtin_cmd_help_handler(int argc, char *argv[]) {
    Console_printf("Available commands:\r\n");
    for (uint8_t i = 0; i < command_count; i++) {
        Console_printf("  %-10s %s\r\n", commands[i]->name, commands[i]->description);
    }
    return 0;
}

/* 版本命令 */
static int console_builtin_cmd_version_handler(int argc, char *argv[]) {
    Console_printf("ZhiyecRTOS Console v1.2\r\n");
    return 0;
}
