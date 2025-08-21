#include "MsgQueue.h"
#include "Defines.h"
#include "Task.h"

extern const struct TaskStruct *const volatile current_task;

/* 初始化消息队列 */
void MsgQueue_init(struct MsgQueue *const msg_queue,
                   const size_t type_size, const void *const buffer, const size_t buffer_size) {
    msg_queue->buffer = buffer;
    msg_queue->buffer_size = buffer_size;
    msg_queue->type_size = type_size;
    msg_queue->timeout = 0U;
    msg_queue->tasks_waiting_to_send = NULL;
    msg_queue->tasks_waiting_to_receive = NULL;
    msg_queue->tasks_count = 0U;
    msg_queue->queue_head = 0U;
    msg_queue->queue_tail = 0U;
    msg_queue->queue_count = 0U;
}

/* 检查队列是否为空 */
static bool MsgQueue_isEmpty(struct MsgQueue *const msg_queue) {
    return (msg_queue->queue_count == 0);
}

/* 检查队列是否已满 */
static bool MsgQueue_isFull(struct MsgQueue *const msg_queue) {
    return (msg_queue->queue_count == msg_queue->buffer_size);
}

/* 发送消息 */
bool MsgQueue_send(struct MsgQueue *const msg_queue, const void *const data) {
    if (msg_queue == NULL || data == NULL) {
        return false;
    }

    while (MsgQueue_isFull(msg_queue)) {
        /* 如果在中断里, 直接返回 */
        if (!Task_isInTask()) {
            return false;
        }

        Task_suspendScheduling();

        struct TaskListNode *const front_node = TaskList_remove(current_task->type);

        if (front_node == NULL) {
            Task_resumeScheduling();
            return false;
        }

        TaskList_push(msg_queue->tasks_waiting_to_send, front_node);

        Task_resumeScheduling();

        yield();
    }

    Task_suspendScheduling();

    /* 往消息队列添加消息 */
    unsigned char *const writer = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_tail);
    for (size_t i = 0; i < msg_queue->type_size; ++i) {
        *(writer + i) = *(((unsigned char *)data) + i);
    }
    msg_queue->queue_tail = (msg_queue->queue_tail + 1) % msg_queue->buffer_size;
    ++(msg_queue->queue_count);

    /* 唤醒等待接收消息的任务 */
    while (true) {
        struct TaskListNode *const node = msg_queue->tasks_waiting_to_receive;
        TaskList_pop(msg_queue->tasks_waiting_to_receive);

        if (node == NULL) {
            break;
        }

        TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
    }

    Task_resumeScheduling();

    return true;
}

/* 接收消息 */
static bool MsgQueue_receiveHelper(struct MsgQueue *const msg_queue, void *const data,
                                   const bool has_timeout) {
    if ((msg_queue == NULL) || (data == NULL)) {
        return false;
    }

    Task_suspendScheduling();
    ++(msg_queue->tasks_count);
    Task_resumeScheduling();

    Tick_t current_tick = Tick_currentTicks();

    /* 没有消息, 进入阻塞 */
    while (MsgQueue_isEmpty(msg_queue)) {
        /* 超时处理 */
        if (has_timeout) {
            if (!Tick_after(Tick_currentTicks(),
                            current_tick + 1)) {
                continue;
            }
            current_tick = Tick_currentTicks();

            if (msg_queue->timeout > 0) {
                --(msg_queue->timeout);
            }
            if (msg_queue->timeout == 0) {
                return false;
            }
            continue;
        }

        Task_suspendScheduling();

        struct TaskListNode *const front_node = TaskList_remove(current_task->type);

        if (front_node == NULL) {
            Task_resumeScheduling();
            return false;
        }

        TaskList_push(msg_queue->tasks_waiting_to_receive, front_node);

        Task_resumeScheduling();

        yield();
    }

    Task_suspendScheduling();

    /* 从消息队列中读取消息 */
    const unsigned char *const reader = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_head);
    for (size_t i = 0; i < msg_queue->type_size; ++i) {
        *(((unsigned char *)data) + i) = *(reader + i);
    }

    --(msg_queue->tasks_count);

    if (msg_queue->tasks_count == 0U) {
        msg_queue->queue_head = (msg_queue->queue_head + 1) % msg_queue->buffer_size;
        --(msg_queue->queue_count);

        /* 唤醒一个等待发送消息的任务 */
        struct TaskListNode *const node = msg_queue->tasks_waiting_to_send;
        TaskList_pop(msg_queue->tasks_waiting_to_send);
        if (node != NULL) {
            TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
        }
    }

    Task_resumeScheduling();

    if (msg_queue->tasks_count != 0U) {
        yield();
    }

    return true;
}

bool MsgQueue_receive(struct MsgQueue *const msg_queue, void *const data) {
    return MsgQueue_receiveHelper(msg_queue, data, false);
}

bool MsgQueue_tryReceive(struct MsgQueue *const msg_queue, void *const data,
                         const Tick_t timeout) {
    msg_queue->timeout = timeout;
    return MsgQueue_receiveHelper(msg_queue, data, true);
}
