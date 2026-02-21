
## 关于
ZhiyecRTOS 是用于嵌入式微控制器 (MCU) 的实时操作系统内核。适配 ARM Cortex M0-M7 架构。

## 使用
1. 克隆仓库到本地
```bash
git clone https://github.com/Zhiyelah/ZhiyecRTOS.git
```
2. 将目录文件添加到 C/C++ 工程中
3. 从 `kernel/arch/` 中选择合适的架构接口文件并放置到 `kernel/` 目录下
4. 根据目标芯片修改 `config.h` 中的配置项 (CPU 时钟频率、Tick 中断频率、内存池大小等) 
5. 参考以下示例创建任务

#### 任务创建示例（使用静态内存分配）
```C
#include <zhiyec/task.h>

#define DO_SOMETHING_TASK_STACK_SIZE 128
stack_t do_something_task_stack[DO_SOMETHING_TASK_STACK_SIZE];
void do_something_task(void *arg) {
    /* 初始化 */
    (void)arg;

    while (true) {
        /* 做些什么 */
    }
}

int main() {
    /* 创建任务 */
    Task_create(do_something_task, NULL,
                do_something_task_stack, DO_SOMETHING_TASK_STACK_SIZE,
                &(struct TaskAttribute){
                    .priority = TASKPRIORITY_MEDIUM,
                });

    /* 开始调度任务 */
    return Task_schedule();
}

```

#### 特别说明
1. 需要 **C99** 及以上标准和 **GNU** 扩展
2. 任务函数 **返回** 后会自动删除该任务
3. 使用动态内存分配时可以通过重载任务的 **.destroy** 方法释放内存
