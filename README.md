
## 说明
Zhiyecontroller RTOS 是用于嵌入式微控制器 (MCU) 的实时操作系统内核。具备轻量、低耦合、高效、可拓展的特点。适配 ARM Cortex M0-M7 架构。

### 功能
- **任务调度**: 支持时间片轮转调度和先进先出调度，实时任务可抢占当前任务，支持任务的创建、删除、挂起、恢复
- **任务间通信**: 提供消息队列、信号量、事件组，满足不同场景下的数据传递需求
- **任务同步**: 使用信号量、互斥锁同步任务
- **系统时基**: 基于内核外设定时器实现系统 Tick，支持定时任务与任务延时
- **内存管理**: 内置内存池与动态堆管理，自动合并内存块

## 使用
1. 克隆仓库到本地
```bash
git clone https://github.com/Zhiyelah/ZhiyecRTOS.git
```
2. 将模块的源文件和头文件导入到 C/C++ 工程中
3. 根据目标芯片修改 `Config.h` 中的配置项 (CPU 时钟频率、Tick 中断频率、内存池大小等) 
4. 参考以下示例创建任务

#### 任务创建示例（使用静态内存分配）
```C
#include <zhiyec/Task.h>

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
    /* 或者也可以这样 */
    /* TaskAttribute_def(do_something_task_attr, do_something_task_stack, COMMON_TASK); */

    /* 创建任务 */
    Task_create(doSomethingTask, NULL, &do_something_task_attr);

    /* 开始执行任务 */
    return Task_exec();
}
```

#### 特别说明
1. 支持 C99 及以上标准
2. 一些模块是可选的，如果不开启 **动态内存分配** 则不需要添加 **`Memory`** 模块
3. 任务函数 **返回** 后会自动删除该任务
