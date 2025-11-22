#include <math.h>
#include <utility/PIDController.h>
#include <zhiyec/Assert.h>

struct PIDController {
    float kp;
    float ki;
    float kd;
    /* PID输出 */
    volatile float pid_output;
    /* 上一次误差 */
    float prev_error;
    /* 累加误差 */
    float sum_error;
};

static_assert(PIDController_byte == sizeof(struct PIDController), "size mismatch");

/* 初始化PID控制器 */
struct PIDController *PIDController_init(void *const pid_mem,
                                         const float kp, const float ki, const float kd) {
    if (!pid_mem) {
        return 0;
    }

    struct PIDController *pid = (struct PIDController *)pid_mem;

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->pid_output = 0;
    pid->prev_error = 0;
    pid->sum_error = 0;
    return pid;
}

/* 计算PID */
float PIDController_calculateOutput(struct PIDController *const pid,
                                    const float error, const float dt) {

    pid->sum_error += error * dt;

    /* 比例项 */
    const float proportional = pid->kp * error;
    /* 积分项 */
    const float integral = pid->ki * pid->sum_error;
    /* 微分项 */
    float derivative = 0.0f;
    if (fabs(dt) > 0.001f) {
        derivative = pid->kd * ((error - pid->prev_error) / dt);
    }

    pid->pid_output = proportional + integral + derivative;

    pid->prev_error = error;

    return pid->pid_output;
}

/* 获取PID输出 */
float PIDController_getOutput(const struct PIDController *const pid) {
    return pid->pid_output;
}
