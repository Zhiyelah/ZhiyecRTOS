/**
 * @file EventGroup.h
 * @author Zhiyelah
 * @brief 事件组
 * @note 可选的模块
 */

#ifndef _EventGroup_h
#define _EventGroup_h

#include "StackList.h"
#include <stdbool.h>
#include <stddef.h>

enum EventTrigLogic {
    EVENT_TRIG_ANY = 0U,
    EVENT_TRIG_ALL = !EVENT_TRIG_ANY,
};

enum EventType {
    EVENT_INPUTDEVICE = 0x01,
    EVENT_TIMER = 0x02,
    EVENT_HARDWAREINT = 0x04,
    EVENT_CUSTOM = 0x08,
};

struct EventGroup {
    /* 事件数 */
    volatile enum EventType events;
    /* 重置的事件数 */
    enum EventType reset_events;
    /* 触发逻辑 */
    enum EventTrigLogic tri_logic;
    /* 等待事件触发的任务 */
    struct StackList tasks_waiting_triggered;
};

/**
 * @brief 初始化事件组
 * @param event_group 事件组对象
 * @param events 事件类型, 可以是下列的其中一个:
 *          EVENT_INPUTDEVICE,
            EVENT_TIMER,
            EVENT_HARDWAREINT,
            EVENT_CUSTOM
 * @param tri_logic 触发逻辑, 可以是下列的其中一个:
            EVENT_TRIG_ANY,
            EVENT_TRIG_ALL
 */
void EventGroup_init(struct EventGroup *const event_group,
                     const enum EventType events, const enum EventTrigLogic tri_logic);

/**
 * @brief 监听事件, 当前任务进入阻塞, 直到事件触发; 监听失败立即返回
 * @param event_group 事件组对象
 * @return 是否监听成功
 */
bool EventGroup_listen(struct EventGroup *const event_group);

/**
 * @brief 触发事件
 * @param event_group 事件组对象
 * @param events 事件类型, 可以是下列的其中一个:
 *          EVENT_INPUTDEVICE,
            EVENT_TIMER,
            EVENT_HARDWAREINT,
            EVENT_CUSTOM
 */
void EventGroup_trigger(struct EventGroup *const event_group, const enum EventType events);

#endif /* _EventGroup_h */
