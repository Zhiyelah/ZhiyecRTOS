#ifndef _ZHIYEC_KERNEL_H
#define _ZHIYEC_KERNEL_H

#include <stddef.h>
#include <zhiyec/types.h>

/**
 * @brief 获取链表节点所在的结构体
 * @param ptr 节点指针
 * @param type 结构体类型
 * @param member 节点在结构体中的成员名称
 */
#define container_of(ptr, type, member) ((type *)((byte *)(ptr) - offsetof(type, member)))

/**
 * @brief 在栈上分配空间
 * @param n_byte 分配的字节
 */
#define ALLOCATE_STACK(n_byte) ((void *)((byte[(n_byte)]){}))

#endif /* _ZHIYEC_KERNEL_H */
