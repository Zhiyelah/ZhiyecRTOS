/**
 * @file PIDController.h
 * @author Zhiyelah
 * @brief PID控制器
 * @note 简单的控制算法
 */

#ifndef _PIDController_h
#define _PIDController_h

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
 * @brief 初始化PID控制器
 * @param pid PID对象
 * @param kp PID比例项参数
 * @param ki PID积分项参数
 * @param kd PID微分项参数
 */
void PIDController_init(struct PIDObject *const pid,
                        const float kp, const float ki, const float kd);

/**
 * @brief 计算误差
 * @param target_value 目标值
 * @param current_value 当前值
 */
float PIDController_calculateError(float target_value, float current_value);

/**
 * @brief 计算PID
 * @param pid PID对象
 * @param error 误差
 * @param dt 时间间隔Δt, 单位: 秒
 */
float PIDController_calculateOutput(struct PIDObject *const pid,
                                    const float error, const float dt);

/**
 * @brief 获取PID输出
 * @param pid PID对象
 */
float PIDController_getOutput(const struct PIDObject *const pid);

#endif /* _PIDController_h */
