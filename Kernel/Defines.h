#ifndef _Defines_h
#define _Defines_h

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#include <stdint.h>

/* 32位系统的栈单位大小 */
typedef uint32_t Stack_t;

/* 滴答计数器类型 */
typedef uint32_t Tick_t;
typedef int32_t STick_t;

#endif /* _Defines_h */
