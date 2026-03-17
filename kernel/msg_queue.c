#include <string.h>
#include <zhiyec/assert.h>
#include <zhiyec/atomic.h>
#include <zhiyec/list.h>
#include <zhiyec/msg_queue.h>
#include <zhiyec/task_list.h>

struct msgqueue {
    /* 数据缓冲区 */
    void *buffer;
    /* 缓冲区大小 */
    size_t buffer_size;
    /* 类型大小 */
    size_t type_size;
    /* 等待发送消息的任务 */
    struct stack_list tasks_waiting_to_send;
    /* 等待接收消息的任务 */
    struct stack_list tasks_waiting_to_receive;
    /* 等待接收消息的任务数 */
    volatile size_t task_waiting_to_receive_count;

    /* 队头指针（出队位置） */
    size_t queue_head;
    /* 队尾指针（入队位置） */
    size_t queue_tail;
    /* 当前队列元素数量 */
    volatile size_t queue_count;
};

static_assert(MSGQUEUE_BYTE == sizeof(struct msgqueue), "size mismatch");

/* 初始化消息队列 */
struct msgqueue *msgqueue_init(void *const msg_queue_mem,
                               const size_t type_size, void *const buffer, const size_t buffer_size) {
    assert(msg_queue_mem != NULL);
    assert(buffer != NULL);

    struct msgqueue *msg_queue = (struct msgqueue *)msg_queue_mem;

    msg_queue->buffer = buffer;
    msg_queue->buffer_size = buffer_size;
    msg_queue->type_size = type_size;
    stack_list_init(msg_queue->tasks_waiting_to_send);
    stack_list_init(msg_queue->tasks_waiting_to_receive);
    msg_queue->task_waiting_to_receive_count = 0U;
    msg_queue->queue_head = 0U;
    msg_queue->queue_tail = 0U;
    msg_queue->queue_count = 0U;

    return msg_queue;
}

/* 检查队列是否为空 */
#define msgqueue_is_empty(msg_queue) ((msg_queue)->queue_count == 0)

/* 检查队列是否已满 */
#define msgqueue_is_full(msg_queue) ((msg_queue)->queue_count == (msg_queue)->buffer_size)

static inline void msgqueue_do_send(struct msgqueue *const msg_queue, const void *const data) {
    /* 往消息队列添加消息 */
    unsigned char *const writer = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_tail);
    memcpy(writer, data, msg_queue->type_size);

    msg_queue->queue_tail = (msg_queue->queue_tail + 1) % msg_queue->buffer_size;
    ++(msg_queue->queue_count);

    /* 唤醒等待接收消息的任务 */
    while (!stack_list_is_empty(msg_queue->tasks_waiting_to_receive)) {
        struct slist_head *const node = stack_list_front(msg_queue->tasks_waiting_to_receive);
        stack_list_pop(msg_queue->tasks_waiting_to_receive);

        tasklist_append(task_get_priority(task_get_from_node(node)), node);
    }
}

/* 发送消息 */
bool msgqueue_send(struct msgqueue *const msg_queue, const void *const data) {
    assert(msg_queue != NULL);
    assert(data != NULL);

    while (true) {
        if (!msgqueue_is_full(msg_queue)) {
            break;
        }

        task_suspend_all();
        struct slist_head *const front_node = tasklist_remove_front(task_get_priority(task_get_current()));

        if (front_node) {
            stack_list_push(msg_queue->tasks_waiting_to_send, front_node);
        }

        task_resume_all();
        task_yield();
    }

    atomic({
        msgqueue_do_send(msg_queue, data);
    });

    return true;
}

bool msgqueue_send_from_isr(struct msgqueue *const msg_queue, const void *const data) {
    assert(msg_queue != NULL);
    assert(data != NULL);

    if (msgqueue_is_full(msg_queue)) {
        return false;
    }

    uint32_t prev_basepri = irq_disable_from_isr();
    msgqueue_do_send(msg_queue, data);
    irq_enable_from_isr(prev_basepri);

    return true;
}

/* 接收消息 */
static bool msgqueue_do_receive(struct msgqueue *const msg_queue, void *const data,
                                const bool has_timeout, tick_t timeout) {
    assert(msg_queue != NULL);
    assert(data != NULL);

    tick_t current_tick = tick_get_current();

    atomic({
        ++(msg_queue->task_waiting_to_receive_count);
    });

    while (true) {
        /* 有消息, 直接接收 */
        if (!msgqueue_is_empty(msg_queue)) {
            break;
        }

        /* 超时处理 */
        if (has_timeout) {

            if (timeout > 0) {
                if (!tick_after(tick_get_current(), current_tick + 1)) {
                    task_yield();
                    continue;
                }

                current_tick = tick_get_current();
                --timeout;
            } else {
                atomic({
                    --(msg_queue->task_waiting_to_receive_count);
                });
                return false;
            }

            continue;
        }

        task_suspend_all();
        struct slist_head *const front_node = tasklist_remove_front(task_get_priority(task_get_current()));

        if (front_node) {
            stack_list_push(msg_queue->tasks_waiting_to_receive, front_node);
        }

        task_resume_all();
        task_yield();
    }

    task_suspend_all();

    atomic({
        --(msg_queue->task_waiting_to_receive_count);
    });

    /* 从消息队列中读取消息 */
    const unsigned char *const reader = (unsigned char *)(msg_queue->buffer) + (msg_queue->type_size * msg_queue->queue_head);
    memcpy(data, reader, msg_queue->type_size);

    if (msg_queue->task_waiting_to_receive_count == 0U) {
        msg_queue->queue_head = (msg_queue->queue_head + 1) % msg_queue->buffer_size;
        --(msg_queue->queue_count);

        /* 唤醒一个等待发送消息的任务 */
        if (!stack_list_is_empty(msg_queue->tasks_waiting_to_send)) {
            struct slist_head *const node = stack_list_front(msg_queue->tasks_waiting_to_send);
            stack_list_pop(msg_queue->tasks_waiting_to_send);

            tasklist_append(task_get_priority(task_get_from_node(node)), node);
        }
    }

    task_resume_all();

    if (msg_queue->task_waiting_to_receive_count != 0U) {
        task_yield();
    }

    return true;
}

void msgqueue_receive(struct msgqueue *const msg_queue, void *const data) {
    (void)msgqueue_do_receive(msg_queue, data, false, 0);
}

bool msgqueue_try_receive(struct msgqueue *const msg_queue, void *const data,
                          const tick_t timeout) {
    return msgqueue_do_receive(msg_queue, data, true, timeout);
}
