#ifndef _ZHIYEC_KERNEL_H
#define _ZHIYEC_KERNEL_H

/**
 * 计算结构体成员偏移
 */
#ifdef offsetof
#undef offsetof
#endif /* offsetof */
#define offsetof(type, member) ((size_t)&(((type *)0)->member))

/**
 * 获取链表容器
 */
#define container_of(ptr, type, member) ((type *)((byte *)(ptr) - offsetof(type, member)))

/* 强制内联 */
#ifdef __forceinline
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif /* FORCEINLINE */

#endif /* _ZHIYEC_KERNEL_H */
