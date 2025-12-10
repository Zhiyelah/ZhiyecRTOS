/**
 * @file Printf.h
 * @author Zhiyelah
 * @brief 格式化打印
 * @note 无需禁用半主机模式、并支持多线程环境的打印输出
 */

#ifndef _Printf_h
#define _Printf_h

/**
 * @brief 设置打印输出函数
 */
void Printf_setOutput(void (*output)(char));

/**
 * @brief 格式化打印
 * @note 线程安全
 */
void Printf(const char *format, ...);

#endif /* _Printf_h */
