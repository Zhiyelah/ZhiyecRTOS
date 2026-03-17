/**
 * @file key_obj.h
 * @author Zhiyelah
 * @brief 多状态按键检测
 */

#ifndef _KEY_OBJ_H
#define _KEY_OBJ_H

#include <stdbool.h>
#include <stddef.h>

enum key_status {
    KEYSTATUS_IDLE = 0,
    KEYSTATUS_PRESS_DELAY,
    KEYSTATUS_RELEASE_DELAY,
    KEYSTATUS_PRESSED,
    KEYSTATUS_LONG_PRESSED,
    KEYSTATUS_DOUBLE_PRESSED
};

struct key_obj {
    /* 按键状态 */
    enum key_status key_status;
    /* 按键上一个状态 */
    enum key_status key_prev_status;
    /* 按键间隔计数 */
    unsigned int key_ticks;

    /* 按键按下判断函数 */
    bool (*is_pressed)(void);
    /* 按键状态改变事件回调 */
    void (*status_changed_event)(enum key_status);
};

/**
 * @brief 按键对象初始化
 * @param key_obj 按键对象
 * @param is_pressed 按键按下判断函数
 * @param status_changed_event 按键状态改变事件回调
 */
void key_init(struct key_obj *key_obj,
              bool (*is_pressed)(void), void (*status_changed_event)(enum key_status));

/**
 * @brief 按键对象轮询
 * @note 更新状态和执行回调
 * @param key_obj 按键对象(数组)
 * @param count 如果是数组则输入数组大小，否则为1
 */
void key_loop(struct key_obj *key_obj, size_t count);

#endif /* _KEY_OBJ_H */
