#ifndef _Printf_h
#define _Printf_h

/**
 * @brief 设置Printf打印输出函数
 */
void Printf_setOutput(void (*output)(char));

/**
 * @brief 线程安全的Printf
 */
void Printf(const char *format, ...);

#endif /* _Printf_h */
