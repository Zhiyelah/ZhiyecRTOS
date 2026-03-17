#include <zhiyec/assert.h>
#include <zhiyec/atomic.h>
#include <zhiyec/event_group.h>
#include <zhiyec/list.h>
#include <zhiyec/task_list.h>

struct eventgroup {
    /* 事件数 */
    volatile enum event_type events;
    /* 重置的事件数 */
    enum event_type reset_events;
    /* 触发逻辑 */
    enum event_trig_logic tri_logic;
    /* 等待事件触发的任务 */
    struct stack_list tasks_waiting_triggered;
};

static_assert(EVENTGROUP_BYTE == sizeof(struct eventgroup), "size mismatch");

/* 初始化事件组 */
struct eventgroup *eventgroup_init(void *const event_group_mem,
                                   const enum event_type events, const enum event_trig_logic tri_logic) {
    assert(event_group_mem != NULL);

    struct eventgroup *event_group = (struct eventgroup *)event_group_mem;

    event_group->events = events;
    event_group->reset_events = events;
    event_group->tri_logic = tri_logic;
    stack_list_init(event_group->tasks_waiting_triggered);

    return event_group;
}

/* 监听事件 */
bool eventgroup_listen(struct eventgroup *const event_group) {
    assert(event_group != NULL);

    while (true) {
        /* 表示事件被触发 */
        if (event_group->events == 0U) {
            break;
        }

        task_suspend_all();
        struct slist_head *const front_node = tasklist_remove_front(task_get_priority(task_get_current()));

        if (front_node) {
            stack_list_push(event_group->tasks_waiting_triggered, front_node);
        }

        task_resume_all();
        task_yield();
    }

    atomic({
        if (stack_list_is_empty(event_group->tasks_waiting_triggered)) {
            event_group->events = event_group->reset_events;
        }
    });

    return true;
}

/* 触发事件 */
void eventgroup_trigger(struct eventgroup *const event_group, const enum event_type events) {
    assert(event_group != NULL);

    /* 该事件已触发或未注册 */
    if ((event_group->events & events) == 0U) {
        return;
    }

    task_suspend_all();

    atomic({
        event_group->events &= ~events;
    });

    if ((event_group->tri_logic == EVENT_TRIG_ANY) ||
        (event_group->events == (enum event_type)0U)) {
        event_group->events = (enum event_type)0U;
        while (!stack_list_is_empty(event_group->tasks_waiting_triggered)) {
            struct slist_head *const node = stack_list_front(event_group->tasks_waiting_triggered);
            stack_list_pop(event_group->tasks_waiting_triggered);

            tasklist_append(task_get_priority(task_get_from_node(node)), node);
        }
    }

    task_resume_all();
}
