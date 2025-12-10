
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

stack_t do_something_task_stack[128];
void doSomethingTask(void *arg) {
    /* 初始化 */
    (void)arg;

    while (true) {
        /* 做些什么 */
    }
}

int main() {
    /* 定义任务属性 */
    struct TaskAttribute do_something_task_attr = {
        .stack = do_something_task_stack,
        .stack_size = sizeof(do_something_task_stack)/sizeof(do_something_task_stack[0]),
        .type = COMMON_TASK, /* COMMON_TASK 或 REALTIME_TASK */
    };
    /* 或者也可以这样来定义任务属性
       TaskAttribute_def(do_something_task_attr, do_something_task_stack, COMMON_TASK); */

    /* 创建任务 */
    Task_create(doSomethingTask, NULL, &do_something_task_attr);

    /* 开始调度任务 */
    return Task_schedule();
}
```

#### 特别说明
1. 支持 C99 及以上标准
2. 一些模块是可选的，如果不开启 **动态内存分配** 则不需要添加 **`Memory`** 模块
3. 任务函数 **返回** 后会自动删除该任务
