#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include <utility/fmt.h>
#include <zhiyec/assert.h>
#include <zhiyec/compiler.h>
#include <zhiyec/kernel.h>
#include <zhiyec/reentrant_lock.h>

#if (USE_DYNAMIC_MEMORY_ALLOCATION)
#include <zhiyec/memory.h>
#endif

static struct PrintStream current_out;
static struct reentrantlock *print_lock = ALLOCATE_STACK(REENTRANTLOCK_BYTE);

void fmt_init(struct PrintStream *print_stream, const enum task_priority print_lock_ceiling_priority) {
    current_out = *print_stream;
    reentrantlock_init(print_lock, print_lock_ceiling_priority);
}

always_inline void fmt_print(const char *str) {
    assert(current_out.write != NULL);

    reentrantlock_lock(print_lock);

    for (size_t i = 0; str[i] != '\0'; ++i) {
        current_out.write(str[i]);
    }

    reentrantlock_unlock(print_lock);
}

void fmt_println(const char *str) {
    assert(current_out.write != NULL);

    reentrantlock_lock(print_lock);

    fmt_print(str);
    current_out.write('\r');
    current_out.write('\n');

    reentrantlock_unlock(print_lock);
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
    char *string = memory_alloc(string_length * sizeof(char));
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
    memory_free(string);
#endif
}
