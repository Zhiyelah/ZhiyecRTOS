/**
 * @file Memory.h
 * @author Zhiyelah
 * @brief 动态内存分配
 * @note 在配置文件中启用动态内存分配功能后添加
 */

#ifndef _ZHIYEC_MEMORY_H
#define _ZHIYEC_MEMORY_H

#include <stddef.h>

/**
 * @brief 分配内存
 * @param size 分配的内存大小
 */
void *Memory_alloc(size_t size);

/**
 * @brief 释放内存
 * @param ptr 通过Memory_alloc分配的内存指针
 */
void Memory_free(void *const ptr);

/**
 * @brief 获取空闲内存大小
 * @return 空闲内存大小
 */
size_t Memory_getFreeSize(void);

#endif /* _ZHIYEC_MEMORY_H */
