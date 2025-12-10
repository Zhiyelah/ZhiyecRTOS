#include <Config.h>
#include <stdarg.h>
#include <stdio.h>
#include <utility/Printf.h>
#include <zhiyec/Kernel.h>
#include <zhiyec/Mutex.h>

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include <zhiyec/Memory.h>
#endif

static void (*printf_output)(char) = NULL;
static struct Mutex *print_lock = ALLOCATE_STACK(Mutex_byte);

void Printf_setOutput(void (*output)(char)) {
    printf_output = output;
    (void)Mutex_init(print_lock);
}

void Printf(const char *format, ...) {
    if (printf_output == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    va_list args_copy;
    va_copy(args_copy, args);
    /* 计算格式化的字符串长度, +1 为包含'\0'的长度 */
    const size_t string_length = vsnprintf(NULL, 0U, format, args_copy) + 1;
    va_end(args_copy);
    char *string = Memory_alloc(string_length * sizeof(char));
    if (string == NULL) {
        return;
    }
#else
    char string[128];
    const size_t string_length = sizeof(string) / sizeof(string[0]);
#endif

    Mutex_lock(print_lock);

    /* 格式化字符串到字符数组中 */
    const int length = vsnprintf(string, string_length, format, args);
    va_end(args);

    /* 字符串过长, 省略 */
    if ((length >= string_length) && (string_length >= 4U)) {
        size_t i = string_length - 1 - 3;
        for (; i < string_length - 1; ++i) {
            string[i] = '.';
        }
        string[i] = '\0';
    }

    for (size_t i = 0; string[i] != '\0'; ++i) {
        printf_output(string[i]);
    }

    Mutex_unlock(print_lock);

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    Memory_free(string);
#endif
}
