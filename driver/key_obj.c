#include <driver/key_obj.h>
#include <zhiyec/assert.h>

/* 防抖时长 */
#define DEBOUNCE_TICKS 15U
/* 长按判定时长 */
#define LONG_PRESSED_TICKS 600U
/* 双击判定间隔时长 */
#define DOUBLE_PRESSED_TICKS 300U

void key_init(struct key_obj *key_obj,
              bool (*is_pressed)(void), void (*status_changed_event)(enum key_status)) {
    assert(key_obj != NULL);
    assert(is_pressed != NULL);
    assert(status_changed_event != NULL);

    key_obj->is_pressed = is_pressed;
    key_obj->status_changed_event = status_changed_event;
    key_obj->key_status = KEYSTATUS_IDLE;
    key_obj->key_prev_status = KEYSTATUS_IDLE;
    key_obj->key_ticks = 0U;
}

static inline void key_update_status(struct key_obj *const key_obj) {
    assert(key_obj != NULL);
    assert(key_obj->is_pressed != NULL);
    assert(key_obj->status_changed_event != NULL);

    const bool is_key_pressed = key_obj->is_pressed();

    enum key_status next_status = key_obj->key_status;
    /* 是否触发事件 */
    bool trigger_event = false;
    /* 触发的事件状态 */
    enum key_status event_status = key_obj->key_status;

    switch (key_obj->key_status) {
    case KEYSTATUS_IDLE:
        if (is_key_pressed) {
            next_status = KEYSTATUS_PRESS_DELAY;
            key_obj->key_ticks = 0U;
        }
        break;

    case KEYSTATUS_PRESS_DELAY:
        key_obj->key_ticks++;
        if (key_obj->key_ticks < DEBOUNCE_TICKS) {
            break;
        }
        key_obj->key_ticks = 0U;

        /* 按键实际未按下 */
        if (!is_key_pressed) {
            next_status = key_obj->key_prev_status;
            break;
        }

        switch (key_obj->key_prev_status) {
        case KEYSTATUS_IDLE: /* 第一次按下按键 */
            next_status = KEYSTATUS_PRESSED;
            break;

        case KEYSTATUS_DOUBLE_PRESSED: /* 第二次按下按键 */
            next_status = KEYSTATUS_LONG_PRESSED;
            trigger_event = true;
            event_status = KEYSTATUS_DOUBLE_PRESSED;
            break;

        default:
            next_status = KEYSTATUS_IDLE;
            break;
        }
        break;

    case KEYSTATUS_RELEASE_DELAY:
        key_obj->key_ticks++;
        if (key_obj->key_ticks < DEBOUNCE_TICKS) {
            break;
        }
        key_obj->key_ticks = 0U;

        /* 按键实际按下了 */
        if (is_key_pressed) {
            next_status = key_obj->key_prev_status;
            break;
        }

        switch (key_obj->key_prev_status) {
        case KEYSTATUS_PRESSED: /* 第一次松开按键 */
            next_status = KEYSTATUS_DOUBLE_PRESSED;
            break;

        case KEYSTATUS_LONG_PRESSED: /* 长按后松开按键 */
            next_status = KEYSTATUS_IDLE;
            trigger_event = true;
            event_status = KEYSTATUS_IDLE;
            break;

        default:
            next_status = KEYSTATUS_IDLE;
            break;
        }
        break;

    case KEYSTATUS_PRESSED:
        if (!is_key_pressed) {
            next_status = KEYSTATUS_RELEASE_DELAY;
            key_obj->key_ticks = 0U;
            break;
        }

        key_obj->key_ticks++;
        /* 按下时长超过长按阈值 */
        if (key_obj->key_ticks >= LONG_PRESSED_TICKS) {
            next_status = KEYSTATUS_LONG_PRESSED;
            trigger_event = true;
            event_status = KEYSTATUS_LONG_PRESSED;
        }
        break;

    /* 长按状态(等待按键松开) */
    case KEYSTATUS_LONG_PRESSED:
        if (!is_key_pressed) {
            next_status = KEYSTATUS_RELEASE_DELAY;
            key_obj->key_ticks = 0U;
        }
        break;

    case KEYSTATUS_DOUBLE_PRESSED:
        /* 双击窗口期内再次按下 */
        if (is_key_pressed) {
            next_status = KEYSTATUS_PRESS_DELAY;
            key_obj->key_ticks = 0U;
            break;
        }

        key_obj->key_ticks++;
        /* 双击窗口期超时 */
        if (key_obj->key_ticks >= DOUBLE_PRESSED_TICKS) {
            next_status = KEYSTATUS_LONG_PRESSED;
            /* 触发普通短按事件 */
            trigger_event = true;
            event_status = KEYSTATUS_PRESSED;
        }
        break;

    default:
        next_status = KEYSTATUS_IDLE;
        break;
    }

    /* 检测状态是否改变 */
    if (next_status != key_obj->key_status) {
        key_obj->key_prev_status = key_obj->key_status;
        key_obj->key_status = next_status;
        if (trigger_event) {
            /* 触发状态变更事件 */
            key_obj->status_changed_event(event_status);
        }
    }
}

void key_loop(struct key_obj *key_obj, size_t count) {
    assert(key_obj != NULL);

    while (count > 0U) {
        key_update_status(key_obj);
        key_obj++;
        count--;
    }
}
