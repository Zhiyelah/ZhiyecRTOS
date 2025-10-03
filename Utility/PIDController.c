#include <utility/PIDController.h>
#include <zhiyec/Assert.h>

struct PIDObject {
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

static_assert(PIDObject_byte == sizeof(struct PIDObject), "size mismatch");

/* 初始化PID控制器 */
struct PIDObject *PIDController_init(void *const pid_mem,
                                     const float kp, const float ki, const float kd) {
    if (!pid_mem) {
        return 0;
    }

    struct PIDObject *pid = (struct PIDObject *)pid_mem;

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->pid_output = 0;
    pid->prev_error = 0;
    pid->sum_error = 0;
    return pid;
}

/* 计算误差 */
float PIDController_calculateError(float target_value, float current_value) {
    float error = target_value - current_value;
    return error;
}

/* 计算PID */
float PIDController_calculateOutput(struct PIDObject *const pid,
                                    const float error, const float dt) {

    pid->sum_error += error * dt;

    /* 比例项 */
    const float proportional = pid->kp * error;
    /* 积分项 */
    const float integral = pid->ki * pid->sum_error;
    /* 微分项 */
    const float derivative = pid->kd * ((error - pid->prev_error) / dt);

    pid->pid_output = proportional + integral + derivative;

    pid->prev_error = error;

    return pid->pid_output;
}

/* 获取PID输出 */
float PIDController_getOutput(const struct PIDObject *const pid) {
    return pid->pid_output;
}
