/**
 * @file EventGroup.h
 * @author Zhiyelah
 * @brief 事件处理
 */

#ifndef _EventGroup_h
#define _EventGroup_h

#include <stdbool.h>

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
    /* 事件类型 */
    enum EventType events;
    /* 剩余未触发事件 */
    enum EventType current_events;
    /* 触发逻辑 */
    enum EventTrigLogic tri_logic;
    /* 第二位表示触发标志 */
    bool is_triggered;
};

/**
 * @brief 创建一个事件组
 * @param event_group 事件组对象
 * @param events 事件类型
 * @param tri_logic 触发逻辑
 */
void EventGroup_new(struct EventGroup *const event_group,
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
 * @param events 事件类型
 */
void EventGroup_trigger(struct EventGroup *const event_group, const enum EventType events);

#endif /* _EventGroup_h */
