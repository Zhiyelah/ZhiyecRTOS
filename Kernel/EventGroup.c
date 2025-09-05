#include "EventGroup.h"
#include "Defines.h"
#include "TaskList.h"

/* 初始化事件组 */
void EventGroup_init(struct EventGroup *const event_group,
                     const enum EventType events, const enum EventTrigLogic tri_logic) {
    event_group->events = events;
    event_group->reset_events = events;
    event_group->tri_logic = tri_logic;
    StackList_init(event_group->tasks_waiting_triggered);
}

/* 监听事件 */
bool EventGroup_listen(struct EventGroup *const event_group) {
    if (event_group == NULL) {
        return false;
    }

    while (true) {
        Task_beginAtomic();
        /* 表示事件被触发 */
        if (event_group->events == 0U) {
            Task_endAtomic();
            break;
        }

        extern const struct TaskStruct *const volatile current_task;
        struct SListHead *const front_node = TaskList_removeFront(current_task->type);

        if (front_node) {
            StackList_push(event_group->tasks_waiting_triggered, front_node);
        }

        Task_endAtomic();
        Task_yield();
    }

    Task_atomic({
        if (StackList_isEmpty(event_group->tasks_waiting_triggered)) {
            event_group->events = event_group->reset_events;
        }
    });

    return true;
}

/* 触发事件 */
void EventGroup_trigger(struct EventGroup *const event_group, const enum EventType events) {
    if (event_group == NULL) {
        return;
    }

    if ((event_group->events & events) == 0U) {
        return;
    }

    Task_atomic({
        event_group->events &= ~events;

        if ((event_group->tri_logic == EVENT_TRIG_ANY) ||
            (event_group->events == (enum EventType)0U)) {
            event_group->events = (enum EventType)0U;
            while (!StackList_isEmpty(event_group->tasks_waiting_triggered)) {
                struct SListHead *const node = StackList_front(event_group->tasks_waiting_triggered);
                StackList_pop(event_group->tasks_waiting_triggered);

                TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
            }
        }
    });
}
