/*
 * drv8313.c — DRV8313 占空比写入实现
 * TIM1 CCR1/2/3 对应 PA8/PA9/PA10（IN1/IN2/IN3）
 * 占空比 1.0 时 CCR=ARR=8499（100% 导通）
 */
#include "drv8313.h"
#include "main.h"

/* CubeMX 生成的 TIM1 句柄 */
extern TIM_HandleTypeDef htim1;

void drv8313_set_duty(float ta, float tb, float tc)
{
    /* clamp 到 [0,1]，防止 PI 异常输出导致占空比越界 */
    if (ta < 0.0f) ta = 0.0f;
    if (ta > 1.0f) ta = 1.0f;
    if (tb < 0.0f) tb = 0.0f;
    if (tb > 1.0f) tb = 1.0f;
    if (tc < 0.0f) tc = 0.0f;
    if (tc > 1.0f) tc = 1.0f;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(ta * DRV8313_PWM_PERIOD));
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)(tb * DRV8313_PWM_PERIOD));
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint32_t)(tc * DRV8313_PWM_PERIOD));
}

void drv8313_off(void)
{
    drv8313_set_duty(0.0f, 0.0f, 0.0f);
}
