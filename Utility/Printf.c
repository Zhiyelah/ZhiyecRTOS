#include "Printf.h"
#include "Interrupt.h"
#include <stdarg.h>
#include <stdio.h>

static void (*printf_output)(char);

void Printf_setOutputFunc(void (*output)(char)) {
    printf_output = output;
}

void Printf(const char *format, ...) {
    char string[256]; // 定义字符数组
    va_list arg;      // 定义可变参数列表数据类型的变量arg
    disable_interrupt_reentrant();
    va_start(arg, format);         // 从format开始，接收参数列表到arg变量
    vsprintf(string, format, arg); // 使用vsprintf打印格式化字符串和参数列表到字符数组中
    va_end(arg);                   // 结束变量arg
    enable_interrupt_reentrant();

    if (printf_output) {
        for (int i = 0; string[i] != '\0'; ++i) {
            printf_output(string[i]);
        }
    }
}
