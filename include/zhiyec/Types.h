/**
 * @file Defines.h
 * @author Zhiyelah
 * @brief 内核定义汇总
 */

#ifndef _Defines_h
#define _Defines_h

#include <stdint.h>

/* 字节类型 */
typedef unsigned char byte;

/* 32位系统的栈单位大小 */
typedef uint32_t stack_t;

/* 滴答计数器类型 */
typedef uint32_t tick_t;
typedef int32_t stick_t;

/* 单向链表 */
struct SListHead {
    struct SListHead *next;
};

/* 双向链表 */
struct ListHead {
    struct ListHead *prev;
    struct ListHead *next;
};

#endif /* _Defines_h */
