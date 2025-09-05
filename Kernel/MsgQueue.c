#include "MsgQueue.h"
#include "Defines.h"
#include "StackList.h"
#include "TaskList.h"

extern const struct TaskStruct *const volatile current_task;

/* 初始化消息队列 */
void MsgQueue_init(struct MsgQueue *const msg_queue,
                   const size_t type_size, void *const buffer, const size_t buffer_size) {
    msg_queue->buffer = buffer;
    msg_queue->buffer_size = buffer_size;
    msg_queue->type_size = type_size;
    StackList_init(msg_queue->tasks_waiting_to_send);
    StackList_init(msg_queue->tasks_waiting_to_receive);
    msg_queue->queue_head = 0U;
    msg_queue->queue_tail = 0U;
    msg_queue->queue_count = 0U;
}

/* 检查队列是否为空 */
#define MsgQueue_isEmpty(msg_queue) ((msg_queue)->queue_count == 0)

/* 检查队列是否已满 */
#define MsgQueue_isFull(msg_queue) ((msg_queue)->queue_count == (msg_queue)->buffer_size)

static inline void MsgQueue_sendHelper(struct MsgQueue *const msg_queue, const void *const data) {
    /* 往消息队列添加消息 */
    unsigned char *const writer = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_tail);
    for (size_t i = 0; i < msg_queue->type_size; ++i) {
        *(writer + i) = *(((unsigned char *)data) + i);
    }
    msg_queue->queue_tail = (msg_queue->queue_tail + 1) % msg_queue->buffer_size;
    ++(msg_queue->queue_count);

    /* 唤醒等待接收消息的任务 */
    while (!StackList_isEmpty(msg_queue->tasks_waiting_to_receive)) {
        struct SListHead *const node = StackList_front(msg_queue->tasks_waiting_to_receive);
        StackList_pop(msg_queue->tasks_waiting_to_receive);

        TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
    }
}

/* 发送消息 */
bool MsgQueue_send(struct MsgQueue *const msg_queue, const void *const data) {
    if (msg_queue == NULL || data == NULL) {
        return false;
    }

    while (true) {
        Task_beginAtomic();
        if (!MsgQueue_isFull(msg_queue)) {
            Task_endAtomic();
            break;
        }

        struct SListHead *const front_node = TaskList_removeFront(current_task->type);

        if (front_node) {
            StackList_push(msg_queue->tasks_waiting_to_send, front_node);
        }

        Task_endAtomic();
        Task_yield();
    }

    Task_atomic({
        MsgQueue_sendHelper(msg_queue, data);
    });

    return true;
}

bool MsgQueue_sendFromISR(struct MsgQueue *const msg_queue, const void *const data) {
    if (msg_queue == NULL || data == NULL) {
        return false;
    }

    if (MsgQueue_isFull(msg_queue)) {
        return false;
    }

    Task_atomic({
        MsgQueue_sendHelper(msg_queue, data);
    });

    return true;
}

/* 接收消息 */
static bool MsgQueue_receiveHelper(struct MsgQueue *const msg_queue, void *const data,
                                   const bool has_timeout, Tick_t timeout) {
    if ((msg_queue == NULL) || (data == NULL)) {
        return false;
    }

    Tick_t current_tick = Tick_currentTicks();

    while (true) {
        Task_beginAtomic();
        /* 有消息, 直接接收 */
        if (!MsgQueue_isEmpty(msg_queue)) {
            Task_endAtomic();
            break;
        }
        Task_endAtomic();

        /* 超时处理 */
        if (has_timeout) {

            if (timeout > 0) {
                if (!Tick_after(Tick_currentTicks(),
                                current_tick + 1)) {
                    Task_yield();
                    continue;
                }

                current_tick = Tick_currentTicks();

                --timeout;
            }

            if (timeout == 0) {
                return false;
            }

            continue;
        }

        Task_atomic({
            struct SListHead *const front_node = TaskList_removeFront(current_task->type);

            if (front_node) {
                StackList_push(msg_queue->tasks_waiting_to_receive, front_node);
            }
        });

        Task_yield();
    }

    Task_atomic({
        /* 从消息队列中读取消息 */
        const unsigned char *const reader = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_head);
        for (size_t i = 0; i < msg_queue->type_size; ++i) {
            *(((unsigned char *)data) + i) = *(reader + i);
        }

        if (StackList_isEmpty(msg_queue->tasks_waiting_to_receive)) {
            msg_queue->queue_head = (msg_queue->queue_head + 1) % msg_queue->buffer_size;
            --(msg_queue->queue_count);

            /* 唤醒一个等待发送消息的任务 */
            if (!StackList_isEmpty(msg_queue->tasks_waiting_to_send)) {
                struct SListHead *const node = StackList_front(msg_queue->tasks_waiting_to_send);
                StackList_pop(msg_queue->tasks_waiting_to_send);

                TaskList_append(container_of(node, struct TaskStruct, node)->type, node);
            }
        }
    });

    if (!StackList_isEmpty(msg_queue->tasks_waiting_to_receive)) {
        Task_yield();
    }

    return true;
}

void MsgQueue_receive(struct MsgQueue *const msg_queue, void *const data) {
    (void)MsgQueue_receiveHelper(msg_queue, data, false, 0);
}

bool MsgQueue_tryReceive(struct MsgQueue *const msg_queue, void *const data,
                         const Tick_t timeout) {
    return MsgQueue_receiveHelper(msg_queue, data, true, timeout);
}
