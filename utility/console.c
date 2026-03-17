#include <string.h>
#include <utility/console.h>

/* 控制台配置参数 */
#define COMMAND_LIST_SIZE 10     // 已注册命令列表大小
#define INPUT_BUFFER_SIZE 128    // 输入缓冲区大小
#define COMMAND_MAX_ARGS 8       // 最大参数数量
#define PROMPT_STRING "zhiyec> " // 命令提示符

#include <utility/fmt.h>
/* 控制台输出 */
#define console_printf(...) fmt_printf(__VA_ARGS__)

/* 输入缓存区 */
static char input_buffer[INPUT_BUFFER_SIZE];
static size_t input_len = 0U;

static int console_builtin_cmd_cmd_handler(int argc, char *argv[]);
static int console_builtin_cmd_help_handler(int argc, char *argv[]);
static int console_builtin_cmd_version_handler(int argc, char *argv[]);

static const struct console_cmd console_builtin_cmd_cmd = {
    .name = "cmd",
    .description = "Start console",
    .handler = console_builtin_cmd_cmd_handler,
};

static const struct console_cmd console_builtin_cmd_help = {
    .name = "help",
    .description = "Show available commands",
    .handler = console_builtin_cmd_help_handler,
};

static const struct console_cmd console_builtin_cmd_version = {
    .name = "version",
    .description = "Show console version",
    .handler = console_builtin_cmd_version_handler,
};

/* 已注册命令列表 */
static const struct console_cmd *commands[COMMAND_LIST_SIZE] = {
    &console_builtin_cmd_cmd,
    &console_builtin_cmd_help,
    &console_builtin_cmd_version,
};
static size_t command_count = 3U;

/* 处理输入缓冲区中的命令 */
static inline void handle_input_buffer(void) {
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
    for (size_t i = 0; i < command_count; ++i) {
        if (strcmp(argv[0], commands[i]->name) == 0) {
            commands[i]->handler(argc, argv);
            return;
        }
    }

    console_printf("Unknown command: %s\r\n", argv[0]);
    console_printf("Type 'help' for command list\r\n");
}

/* 输入字符 */
void console_input_char(char ch) {
    if (ch == '\r') { /* 回车 */
        /* 回显换行 */
        console_printf("\r\n");

        /* 处理输入缓存区 */
        input_buffer[input_len] = '\0';
        handle_input_buffer();

        /* 清空输入缓存区 */
        input_len = 0;
        memset(input_buffer, 0, INPUT_BUFFER_SIZE);

        /* 打印命令提示符 */
        console_printf(PROMPT_STRING);

    } else if (ch == '\b' && input_len > 0) { /* 退格 */
        /* 回退缓存区 */
        input_buffer[--input_len] = '\0';

        /* 清除显示的字符 */
        console_printf("\b \b");

    } else if (input_len < INPUT_BUFFER_SIZE - 1) { /* 普通字符 */
        /* 回显输入字符 */
        console_printf("%c", ch);

        /* 加入缓存区 */
        input_buffer[input_len++] = ch;
    }
}

/* 注册命令 */
void console_register_cmd(const struct console_cmd *const cmd) {
    if (command_count < COMMAND_LIST_SIZE) {
        commands[command_count++] = cmd;
    } else {
        console_printf("Command list full, cannot register %s\r\n", cmd->name);
    }
}

/* 内置命令实现 */

/* 打开控制台 */
static int console_builtin_cmd_cmd_handler(int argc, char *argv[]) {
    /* 打印命令提示符 */
    console_printf(PROMPT_STRING);
    return 0;
}

/* 帮助命令 */
static int console_builtin_cmd_help_handler(int argc, char *argv[]) {
    console_printf("Available commands:\r\n");
    for (size_t i = 0; i < command_count; ++i) {
        console_printf("  %-10s %s\r\n", commands[i]->name, commands[i]->description);
    }
    return 0;
}

/* 版本命令 */
static int console_builtin_cmd_version_handler(int argc, char *argv[]) {
    console_printf("ZhiyecRTOS Console v1.2\r\n");
    return 0;
}
