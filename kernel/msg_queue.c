#include <../kernel/atomic.h>
#include <../kernel/task_list.h>
#include <string.h>
#include <zhiyec/assert.h>
#include <zhiyec/list.h>
#include <zhiyec/msg_queue.h>

struct MsgQueue {
    /* 数据缓冲区 */
    void *buffer;
    /* 缓冲区大小 */
    size_t buffer_size;
    /* 类型大小 */
    size_t type_size;
    /* 等待发送消息的任务 */
    struct StackList tasks_waiting_to_send;
    /* 等待接收消息的任务 */
    struct StackList tasks_waiting_to_receive;
    /* 等待接收消息的任务数 */
    size_t task_count;

    /* 队头指针（出队位置） */
    size_t queue_head;
    /* 队尾指针（入队位置） */
    size_t queue_tail;
    /* 当前队列元素数量 */
    volatile size_t queue_count;
};

static_assert(MsgQueue_byte == sizeof(struct MsgQueue), "size mismatch");

/* 初始化消息队列 */
struct MsgQueue *MsgQueue_init(void *const msg_queue_mem,
                               const size_t type_size, void *const buffer, const size_t buffer_size) {
    if (!msg_queue_mem) {
        return NULL;
    }

    struct MsgQueue *msg_queue = (struct MsgQueue *)msg_queue_mem;

    msg_queue->buffer = buffer;
    msg_queue->buffer_size = buffer_size;
    msg_queue->type_size = type_size;
    StackList_init(msg_queue->tasks_waiting_to_send);
    StackList_init(msg_queue->tasks_waiting_to_receive);
    msg_queue->task_count = 0U;
    msg_queue->queue_head = 0U;
    msg_queue->queue_tail = 0U;
    msg_queue->queue_count = 0U;
    return msg_queue;
}

/* 检查队列是否为空 */
#define MsgQueue_isEmpty(msg_queue) ((msg_queue)->queue_count == 0)

/* 检查队列是否已满 */
#define MsgQueue_isFull(msg_queue) ((msg_queue)->queue_count == (msg_queue)->buffer_size)

static inline void MsgQueue_doSend(struct MsgQueue *const msg_queue, const void *const data) {
    /* 往消息队列添加消息 */
    unsigned char *const writer = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_tail);
    memcpy(writer, data, msg_queue->type_size);

    msg_queue->queue_tail = (msg_queue->queue_tail + 1) % msg_queue->buffer_size;
    ++(msg_queue->queue_count);

    /* 唤醒等待接收消息的任务 */
    while (!StackList_isEmpty(msg_queue->tasks_waiting_to_receive)) {
        struct SListHead *const node = StackList_front(msg_queue->tasks_waiting_to_receive);
        StackList_pop(msg_queue->tasks_waiting_to_receive);

        TaskList_append(Task_getType(Task_fromTaskNode(node)), node);
    }
}

/* 发送消息 */
bool MsgQueue_send(struct MsgQueue *const msg_queue, const void *const data) {
    if (msg_queue == NULL || data == NULL) {
        return false;
    }

    while (true) {
        Atomic_begin();
        if (!MsgQueue_isFull(msg_queue)) {
            Atomic_end();
            break;
        }

        struct SListHead *const front_node = TaskList_removeFront(Task_getType(Task_currentTask()));

        if (front_node) {
            StackList_push(msg_queue->tasks_waiting_to_send, front_node);
        }

        Atomic_end();
        Task_yield();
    }

    atomic({
        MsgQueue_doSend(msg_queue, data);
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

    uint32_t prev_basepri = Port_disableInterruptFromISR();
    MsgQueue_doSend(msg_queue, data);
    Port_enableInterruptFromISR(prev_basepri);

    return true;
}

/* 接收消息 */
static bool MsgQueue_doReceive(struct MsgQueue *const msg_queue, void *const data,
                               const bool has_timeout, tick_t timeout) {
    if ((msg_queue == NULL) || (data == NULL)) {
        return false;
    }

    tick_t current_tick = Tick_current();

    atomic({
        ++(msg_queue->task_count);
    });

    while (true) {
        Atomic_begin();
        /* 有消息, 直接接收 */
        if (!MsgQueue_isEmpty(msg_queue)) {
            Atomic_end();
            break;
        }
        Atomic_end();

        /* 超时处理 */
        if (has_timeout) {

            if (timeout > 0) {
                if (!Tick_after(Tick_current(),
                                current_tick + 1)) {
                    Task_yield();
                    continue;
                }

                current_tick = Tick_current();

                --timeout;
            }

            if (timeout == 0) {
                atomic({
                    --(msg_queue->task_count);
                });
                return false;
            }

            continue;
        }

        atomic({
            struct SListHead *const front_node = TaskList_removeFront(Task_getType(Task_currentTask()));

            if (front_node) {
                StackList_push(msg_queue->tasks_waiting_to_receive, front_node);
            }
        });

        Task_yield();
    }

    atomic({
        --(msg_queue->task_count);

        /* 从消息队列中读取消息 */
        const unsigned char *const reader = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_head);
        memcpy(data, reader, msg_queue->type_size);

        if (msg_queue->task_count == 0U) {
            msg_queue->queue_head = (msg_queue->queue_head + 1) % msg_queue->buffer_size;
            --(msg_queue->queue_count);

            /* 唤醒一个等待发送消息的任务 */
            if (!StackList_isEmpty(msg_queue->tasks_waiting_to_send)) {
                struct SListHead *const node = StackList_front(msg_queue->tasks_waiting_to_send);
                StackList_pop(msg_queue->tasks_waiting_to_send);

                TaskList_append(Task_getType(Task_fromTaskNode(node)), node);
            }
        }
    });

    if (msg_queue->task_count != 0U) {
        Task_yield();
    }

    return true;
}

void MsgQueue_receive(struct MsgQueue *const msg_queue, void *const data) {
    (void)MsgQueue_doReceive(msg_queue, data, false, 0);
}

bool MsgQueue_tryReceive(struct MsgQueue *const msg_queue, void *const data,
                         const tick_t timeout) {
    return MsgQueue_doReceive(msg_queue, data, true, timeout);
}
