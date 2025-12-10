/**
 * @file types.h
 * @author Zhiyelah
 * @brief 类型定义
 */

#ifndef _ZHIYEC_TYPES_H
#define _ZHIYEC_TYPES_H

/* 字节类型 */
typedef unsigned char byte;

/* 栈单位大小 */
typedef unsigned long stack_t;

/* 滴答计数器类型 */
typedef unsigned long tick_t;
typedef signed long stick_t;

/* 单向链表 */
struct SListHead {
    struct SListHead *next;
};

/* 双向链表 */
struct ListHead {
    struct ListHead *prev;
    struct ListHead *next;
};

#endif /* _ZHIYEC_TYPES_H */
