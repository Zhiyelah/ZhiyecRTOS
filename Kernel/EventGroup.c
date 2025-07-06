#include "EventGroup.h"
#include "Task.h"
#include "TaskList.h"
#include <stddef.h>

/* 创建一个事件组 */
void EventGroup_new(struct EventGroup *const event_group,
                    const enum EventType events, const enum EventTrigLogic tri_logic) {
    event_group->events = events;
    event_group->current_events = events;
    event_group->tri_logic = tri_logic;
    event_group->is_triggered = false;
}

/* 监听事件 */
bool EventGroup_listen(struct EventGroup *const event_group) {
    if (event_group == NULL) {
        return false;
    }
    while (!event_group->is_triggered) {
        Task_suspendScheduling();
        struct TaskStruct *const task = TaskList_moveFrontActiveTaskToEventList();
        Task_resumeScheduling();

        if (task == NULL) {
            return false;
        }

        Task_suspendScheduling();
        task->suspension_reason.event_group = event_group;
        task->suspension_reason.event_group->current_events = task->suspension_reason.event_group->events;
        Task_resumeScheduling();

        yield();
    }

    Task_suspendScheduling();
    event_group->is_triggered = false;
    Task_resumeScheduling();

    return true;
}

/* 触发事件 */
void EventGroup_trigger(struct EventGroup *const event_group, const enum EventType events) {
    if (event_group == NULL) {
        return;
    }
    Task_suspendScheduling();
    for (struct TaskListNode *prev_node = NULL, *node = TaskList_getFrontEventTaskNode();
         node != NULL; prev_node = node, node = node->next) {
        if ((node->task->suspension_reason.event_group == event_group) &&
            (node->task->suspension_reason.event_group->current_events & events)) {
            node->task->suspension_reason.event_group->current_events &= ~events;

            if ((node->task->suspension_reason.event_group->tri_logic == EVENT_TRIG_ANY) ||
                (node->task->suspension_reason.event_group->current_events == 0U)) {
                node->task->suspension_reason.event_group = NULL;
                TaskList_putEventTaskToActiveListBack(prev_node,
                                                      node);
            }
        }
    }
    Task_resumeScheduling();

    Task_suspendScheduling();
    event_group->is_triggered = true;
    Task_resumeScheduling();
}
