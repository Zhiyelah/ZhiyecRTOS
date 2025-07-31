#ifndef _SingleKey_h
#define _SingleKey_h

#include <stdbool.h>

enum KeyStatus {
    KeyStatus_Idle = 0,
    KeyStatus_Delay,
    KeyStatus_Pressed,
    KeyStatus_LongPressed
};

struct SingleKey {
    /* 按键状态 */
    enum KeyStatus key_status;
    /* 按键计数 */
    unsigned int key_count;

    /* 按键按下条件 */
    bool (*is_pressed)(void);
    /* 当按键按下时执行 */
    void (*on_pressed)(void);
    /* 当按键长按时执行 */
    void (*on_long_pressed)(void);
};

void Key_pressedEvent(struct SingleKey *single_key);

#endif /* _SingleKey_h */
