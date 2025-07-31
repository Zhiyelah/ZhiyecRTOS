#include "PIDControler.h"

/* 创建一个PID控制器 */
void PIDControler_new(struct PIDObject *const pid_object,
                      const float kp, const float ki, const float kd) {
    pid_object->kp = kp;
    pid_object->ki = ki;
    pid_object->kd = kd;
    pid_object->pid_output = 0;
    pid_object->prev_error = 0;
    pid_object->sum_error = 0;
}

/* 计算误差 */
float PIDControler_calculateError(float target_value, float current_value) {
    float error = target_value - current_value;
    return error;
}

/* 计算PID */
float PIDControler_calculatePID(struct PIDObject *const pid_object,
                                const float error, const float dt) {

    pid_object->sum_error += error * dt;

    /* 比例项 */
    const float proportional = pid_object->kp * error;
    /* 积分项 */
    const float integral = pid_object->ki * pid_object->sum_error;
    /* 微分项 */
    const float derivative = pid_object->kd * ((error - pid_object->prev_error) / dt);

    pid_object->pid_output = proportional + integral + derivative;

    pid_object->prev_error = error;

    return pid_object->pid_output;
}

/* 获取PID输出 */
float PIDControler_getPIDOutput(const struct PIDObject *const pid_object) {
    return pid_object->pid_output;
}
