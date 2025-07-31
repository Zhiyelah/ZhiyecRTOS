#ifndef _PIDControler_h
#define _PIDControler_h

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

/**
 * @brief 创建一个PID控制器
 * @param pid_object PID对象
 * @param kp PID比例项参数
 * @param ki PID积分项参数
 * @param kd PID微分项参数
 */
void PIDControler_new(struct PIDObject *const pid_object,
                      const float kp, const float ki, const float kd);

/**
 * @brief 计算误差
 * @param target_value 目标值
 * @param current_value 当前值
 */
float PIDControler_calculateError(float target_value, float current_value);

/**
 * @brief 计算PID
 * @param pid_object PID对象
 * @param error 误差
 * @param dt 时间间隔Δt, 单位: 秒
 */
float PIDControler_calculatePID(struct PIDObject *const pid_object,
                                const float error, const float dt);

/**
 * @brief 获取PID输出
 * @param pid_object PID对象
 */
float PIDControler_getPIDOutput(const struct PIDObject *const pid_object);

#endif /* _PIDControler_h */
