#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include <utility/fmt.h>
#include <zhiyec/compiler.h>
#include <zhiyec/kernel.h>
#include <zhiyec/reentrant_lock.h>

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include <zhiyec/memory.h>
#endif

static struct PrintStream current_out;
static struct ReentrantLock *print_lock = ALLOCATE_STACK(ReentrantLock_byte);

void fmt_setOut(struct PrintStream *print_stream) {
    current_out = *print_stream;
    ReentrantLock_init(print_lock);
}

always_inline void fmt_print(const char *str) {
    if (current_out.write == NULL) {
        return;
    }

    ReentrantLock_lock(print_lock);

    for (size_t i = 0; str[i] != '\0'; ++i) {
        current_out.write(str[i]);
    }

    ReentrantLock_unlock(print_lock);
}

void fmt_println(const char *str) {
    if (current_out.write == NULL) {
        return;
    }

    ReentrantLock_lock(print_lock);

    fmt_print(str);
    current_out.write('\r');
    current_out.write('\n');

    ReentrantLock_unlock(print_lock);
}

void fmt_printf(const char *format, ...) {
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

    fmt_print(string);

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
    Memory_free(string);
#endif
}
