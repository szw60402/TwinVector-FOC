/*
 * drv8313.h — DRV8313 三相驱动（3-PWM 模式）封装
 * 硬件接线：
 *   VM → 上红轨 11.4V（硅胶线直连，不走面包板铜箔）
 *   GND → 上蓝轨
 *   EN → 3.3V（硬件直接使能，代码不控制）
 *   IN1/IN2/IN3 ← G431 PA8/PA9/PA10（TIM1 CH1/CH2/CH3，20kHz）
 *   OUT1/OUT2/OUT3 → 电机三相（任意接，反转换两根）
 *   模块 3.3V 脚悬空（内部 LDO 输出）
 *
 * 3-PWM 模式：DRV8313 内部处理死区，无需互补 PWM
 */
#ifndef DRV8313_H
#define DRV8313_H

#include <stdint.h>

/* PWM 周期计数（与 CubeMX ARR=8499 对应） */
#define DRV8313_PWM_PERIOD  8499

/* 设置三相占空比（0.0 ~ 1.0）。立即写入 TIM1 CCR，下个周期生效 */
void drv8313_set_duty(float ta, float tb, float tc);

/* 关断三相输出（占空比全 0） */
void drv8313_off(void);

#endif /* DRV8313_H */
