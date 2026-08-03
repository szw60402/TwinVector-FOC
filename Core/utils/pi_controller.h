/*
 * pi_controller.h — 抗积分饱和 PI 控制器
 * 硬件相关：无
 *
 * 位置式 PI：u = kp*err + ki*Σerr（10kHz 中断内每 tick 调用一次）
 * 抗饱和：积分项 clamp 到 ±integral_limit；输出 clamp 到 ±out_limit
 * 条件积分：输出饱和时停止积分，防止 windup
 */
#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

typedef struct {
    float kp;               /* 比例系数 */
    float ki;               /* 积分系数（每 tick 累积） */
    float integral;         /* 积分累积值 */
    float out_limit;        /* 输出限幅（绝对值） */
    float integral_limit;   /* 积分限幅（绝对值），防 windup */
} pi_controller_t;

/* 初始化：清零积分，设置系数与限幅 */
void pi_init(pi_controller_t *pi, float kp, float ki, float out_limit, float integral_limit);

/* 单步更新：输入误差 err，返回输出。10kHz 下每 tick 调用 */
float pi_update(pi_controller_t *pi, float err);

/* 复位积分（换挡/模式切换时调用，避免跳变） */
void pi_reset(pi_controller_t *pi);

#endif /* PI_CONTROLLER_H */
