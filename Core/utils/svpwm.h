/*
 * svpwm.h — 空间矢量 PWM（SVPWM）占空比计算
 * 硬件相关：无（纯数学）
 *
 * 输入：αβ 电压指令（伏特），母线电压 vdc（伏特）
 * 输出：三相归一化占空比 ta/tb/tc ∈ [0,1]，直接写 TIM1 CCR
 *
 * 原理：SVPWM = 逆 Park 后的 αβ 矢量用相邻两个基本矢量合成。
 * 实现采用扇区判断 + 作用时间公式，数学上与 min-max 零序注入等价。
 */
#ifndef SVPWM_H
#define SVPWM_H

/* 计算三相占空比。vdc 为母线电压，valpha/vbeta 为电压指令 */
void svpwm_calc(float valpha, float vbeta, float vdc, float *ta, float *tb, float *tc);

#endif /* SVPWM_H */
