#include <../kernel/atomic.h>
#include <../kernel/task_list.h>
#include <zhiyec/assert.h>
#include <zhiyec/event_group.h>
#include <zhiyec/list.h>

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

static_assert(EventGroup_byte == sizeof(struct EventGroup), "size mismatch");

/* 初始化事件组 */
struct EventGroup *EventGroup_init(void *const event_group_mem,
                                   const enum EventType events, const enum EventTrigLogic tri_logic) {
    if (!event_group_mem) {
        return NULL;
    }

    struct EventGroup *event_group = (struct EventGroup *)event_group_mem;

    event_group->events = events;
    event_group->reset_events = events;
    event_group->tri_logic = tri_logic;
    StackList_init(event_group->tasks_waiting_triggered);
    return event_group;
}

/* 监听事件 */
bool EventGroup_listen(struct EventGroup *const event_group) {
    if (event_group == NULL) {
        return false;
    }

    while (true) {
        Atomic_begin();
        /* 表示事件被触发 */
        if (event_group->events == 0U) {
            Atomic_end();
            break;
        }

        struct SListHead *const front_node = TaskList_removeFront(Task_getType(Task_currentTask()));

        if (front_node) {
            StackList_push(event_group->tasks_waiting_triggered, front_node);
        }

        Atomic_end();
        Task_yield();
    }

    atomic({
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

    atomic({
        event_group->events &= ~events;

        if ((event_group->tri_logic == EVENT_TRIG_ANY) ||
            (event_group->events == (enum EventType)0U)) {
            event_group->events = (enum EventType)0U;
            while (!StackList_isEmpty(event_group->tasks_waiting_triggered)) {
                struct SListHead *const node = StackList_front(event_group->tasks_waiting_triggered);
                StackList_pop(event_group->tasks_waiting_triggered);

                TaskList_append(Task_getType(Task_fromTaskNode(node)), node);
            }
        }
    });
}
