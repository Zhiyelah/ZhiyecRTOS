#include "MsgQueue.h"
#include "Task.h"
#include "TaskList.h"
#include <stddef.h>

/* 创建一个消息队列 */
void MsgQueue_new(struct MsgQueue *const msg_queue,
                  const size_t type, void *const buffer, const size_t buffer_size) {
    msg_queue->type = type;
    msg_queue->buffer = buffer;
    msg_queue->buffer_size = buffer_size;
    msg_queue->head = 0;
    msg_queue->tail = 0;
    msg_queue->count = 0;
}

/* 检查队列是否为空 */
static bool MsgQueue_isEmpty(struct MsgQueue *const msg_queue) {
    return (msg_queue->count == 0);
}

/* 检查队列是否已满 */
static bool MsgQueue_isFull(struct MsgQueue *const msg_queue) {
    return (msg_queue->count == msg_queue->buffer_size);
}

/* 发送消息 */
bool MsgQueue_send(struct MsgQueue *const msg_queue, void *const data) {
    Task_suspendScheduling();
    for (struct TaskListNode *prev_node = NULL, *node = TaskList_getFrontMessageTaskNode();
         node != NULL; prev_node = node, node = node->next) {
        if (node->task->suspension_reason.msg_queue == msg_queue) {
            node->task->suspension_reason.msg_queue = NULL;

            TaskList_putMessageTaskToActiveListBack(prev_node,
                                                    node);
        }
    }
    Task_resumeScheduling();

    if (MsgQueue_isFull(msg_queue)) {
        return false;
    }

    Task_suspendScheduling();
    unsigned char *const writer = (unsigned char *)msg_queue->buffer + (msg_queue->type * msg_queue->tail);
    for (size_t i = 0; i < msg_queue->type; ++i) {
        *(writer + i) = *((unsigned char *)data + i);
    }
    msg_queue->tail = (msg_queue->tail + 1) % msg_queue->buffer_size;
    msg_queue->count++;
    Task_resumeScheduling();

    return true;
}

/* 接收消息 */
bool MsgQueue_receive(struct MsgQueue *const msg_queue, void *const data) {
    while (MsgQueue_isEmpty(msg_queue)) {
        Task_suspendScheduling();
        struct TaskStruct *const task = TaskList_moveFrontActiveTaskToMessageList();
        Task_resumeScheduling();

        if (task == NULL) {
            return false;
        }

        Task_suspendScheduling();
        task->suspension_reason.msg_queue = msg_queue;
        Task_resumeScheduling();

        yield();
    }

    Task_suspendScheduling();
    const unsigned char *const reader = (unsigned char *)msg_queue->buffer + (msg_queue->type * msg_queue->head);
    for (size_t i = 0; i < msg_queue->type; ++i) {
        *((unsigned char *)data + i) = *(reader + i);
    }
    msg_queue->head = (msg_queue->head + 1) % msg_queue->buffer_size;
    msg_queue->count--;
    Task_resumeScheduling();

    return true;
}
