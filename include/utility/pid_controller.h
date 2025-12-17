/**
 * @file pid_controller.h
 * @author Zhiyelah
 * @brief PID控制器
 * @note 简单的控制算法
 */

#ifndef _PID_CONTROLLER_H
#define _PID_CONTROLLER_H

struct PIDController;

#define PIDController_byte 24

/**
 * @brief 初始化PID控制器
 * @param pid_mem 对象内存指针
 * @param kp PID比例项参数
 * @param ki PID积分项参数
 * @param kd PID微分项参数
 * @return 对象指针
 */
struct PIDController *PIDController_init(void *const pid_mem,
                                         const float kp, const float ki, const float kd);

/**
 * @brief 计算误差
 * @param target_value 目标值
 * @param current_value 当前值
 * @return 误差
 */
#define PIDController_calculateError(target_value, current_value) \
    ((float)(target_value - current_value))

/**
 * @brief 计算PID
 * @param pid PID对象
 * @param error 误差
 * @param dt 时间间隔Δt, 单位: 秒
 * @return PID输出
 */
float PIDController_calculateOutput(struct PIDController *const pid,
                                    const float error, const float dt);

/**
 * @brief 获取PID输出
 * @param pid PID对象
 * @return PID输出
 */
float PIDController_getOutput(const struct PIDController *const pid);

#endif /* _PID_CONTROLLER_H */
