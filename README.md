
## ZhiyecRTOS - Zhiye controller RTOS
一个专为嵌入式 MCU 设计的轻量实时操作系统 (RTOS)。支持 STM32 等 ARM Cortex-M 架构的 MCU。

## 快速开始
### 1. 克隆仓库到本地
```bash
git clone https://github.com/Zhiyelah/ZhiyecRTOS.git
```
### 2. 添加到工程
1. 将 `include` 添加到编译器的头文件搜索路径。
2. 将 `kernel/` 下的所有源文件添加到工程编译列表。
3. 根据目标芯片，从 `arch/` 中选择对应的架构文件 (`port.c`、`isr.c`) 添加到工程。
4. 将 `arch/<架构>/include` 也添加到头文件搜索路径。

### 3. 配置系统
根据目标芯片修改 `config.h` 中的配置项：

- CPU时钟频率
- Tick频率
- 内存池大小
- 其他非关键参数

### 4. 创建任务
#### 任务创建示例（使用静态内存分配）
```C
#include <zhiyec/task.h>

#define DO_SOMETHING_TASK_STACK_SIZE 128
stack_t do_something_task_stack[DO_SOMETHING_TASK_STACK_SIZE];
void do_something_task(void *arg) {
    /* 初始化 */

    while (true) {
        /* 做些什么 */
    }
}

int main() {
    /* 创建任务 */
    task_create(
        do_something_task,                  /* 任务函数 */
        NULL,                               /* 任务函数参数 */
        do_something_task_stack,            /* 任务栈地址 */
        DO_SOMETHING_TASK_STACK_SIZE,       /* 任务栈大小 */
        &(struct task_attribute){
            .priority = TASKPRIO_MEDIUM,    /* 任务优先级 */
        }
    );

    /* 开始调度任务 */
    return task_schedule();
}

```

#### 特别说明
1. 需要 **C99** 及以上标准和 **GNU** 扩展
2. 任务函数 **返回** 后会自动删除该任务
3. 使用动态内存分配时可以通过重载任务的 **.destroy** 方法释放内存
