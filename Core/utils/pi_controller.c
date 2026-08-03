/*
 * pi_controller.c — PI 实现
 * 位置式 + 条件积分抗饱和：
 *   正常时 integral += ki*err
 *   输出饱和且积分方向与饱和方向一致时冻结积分
 * 复位函数用于模式切换（开环→闭环）时清积分防跳变
 */
#include "pi_controller.h"

void pi_init(pi_controller_t *pi, float kp, float ki, float out_limit, float integral_limit)
{
    pi->kp = kp;
    pi->ki = ki;
    pi->integral = 0.0f;
    pi->out_limit = out_limit;
    pi->integral_limit = integral_limit;
}

float pi_update(pi_controller_t *pi, float err)
{
    float out;

    /* 积分先按上限 clamp（防止在冻结判断前就溢出） */
    pi->integral += pi->ki * err;
    if (pi->integral > pi->integral_limit) {
        pi->integral = pi->integral_limit;
    } else if (pi->integral < -pi->integral_limit) {
        pi->integral = -pi->integral_limit;
    }

    out = pi->kp * err + pi->integral;

    /* 输出 clamp + 条件积分：饱和时若积分仍往同方向推则回退积分 */
    if (out > pi->out_limit) {
        out = pi->out_limit;
        if (pi->integral > pi->out_limit - pi->kp * err) {
            pi->integral = pi->out_limit - pi->kp * err;
        }
    } else if (out < -pi->out_limit) {
        out = -pi->out_limit;
        if (pi->integral < -pi->out_limit - pi->kp * err) {
            pi->integral = -pi->out_limit - pi->kp * err;
        }
    }

    return out;
}

void pi_reset(pi_controller_t *pi)
{
    pi->integral = 0.0f;
}
