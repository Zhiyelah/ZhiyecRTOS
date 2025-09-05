#ifndef _Printf_h
#define _Printf_h

/**
 * @brief 设置打印输出函数
 */
void Printf_setOutput(void (*output)(char));

/**
 * @brief 格式化打印
 */
void Printf(const char *format, ...);

#endif /* _Printf_h */
