/*
 * svpwm.c — SVPWM 实现
 *
 * 算法：标准扇区法
 *   u1 = vbeta, u2 = 0.866*valpha + 0.5*vbeta, u3 = -0.866*valpha + 0.5*vbeta
 *   扇区 = sign 组合 → 查相邻矢量，计算 T1/T2 → 归一化到 [0,1]
 *
 * 限制：|V| 超过六边形内切圆（Vdc/√3）时按比例缩放（过调制保护），
 *       防止 PI 饱和时占空比超界导致相电流畸变。
 */
#include "svpwm.h"
#include <stdint.h>

static const float SQRT3 = 1.73205080757f;

void svpwm_calc(float valpha, float vbeta, float vdc, float *ta, float *tb, float *tc)
{
    /* 归一化电压：以 Vdc/2 为基准（伏特 → 无量纲）。
       SVPWM 最大线性区 = 1/sqrt(3) ≈ 0.577（对应相电压 Vdc/√3） */
    float k = 2.0f / vdc;
    float v1 = vbeta * k;
    float v2 = ((SQRT3 * 0.5f) * valpha - 0.5f * vbeta) * k;
    float v3 = (-(SQRT3 * 0.5f) * valpha - 0.5f * vbeta) * k;

    /* 扇区判断（sector 1~6） */
    uint8_t sector = 0;
    if (v1 > 0.0f) sector |= 0x01;
    if (v2 > 0.0f) sector |= 0x02;
    if (v3 > 0.0f) sector |= 0x04;

    uint8_t sec;
    switch (sector) {
        case 3: sec = 1; break;   /* v1+,v2+,v3- */
        case 1: sec = 2; break;   /* v1+,v2-,v3- */
        case 5: sec = 3; break;   /* v1+,v2-,v3+ */
        case 4: sec = 4; break;   /* v1-,v2-,v3+ */
        case 6: sec = 5; break;   /* v1-,v2+,v3+ */
        case 2: sec = 6; break;   /* v1-,v2+,v3- */
        default: sec = 0; break;  /* 零矢量 */
    }

    float t1, t2;  /* 相邻矢量作用时间（归一化） */

    switch (sec) {
        case 1: t1 =  v3; t2 =  v1; break;
        case 2: t1 =  v2; t2 = -v3; break;
        case 3: t1 = -v1; t2 =  v2; break;
        case 4: t1 = -v3; t2 = -v1; break;
        case 5: t1 = -v2; t2 =  v3; break;
        case 6: t1 =  v1; t2 = -v2; break;
        default: t1 = 0.0f; t2 = 0.0f; break;
    }

    /* 过调制保护：作用时间总和超 1 时整体缩放（保持方向、限幅幅值） */
    float tsum = t1 + t2;
    if (tsum > 1.0f) {
        float scale = 1.0f / tsum;
        t1 *= scale;
        t2 *= scale;
        tsum = 1.0f;
    }

    /* 七段式：零矢量均分在两端和中间，降低开关频率与谐波 */
    float t0 = 1.0f - tsum;
    float t_a, t_b, t_c;

    switch (sec) {
        case 1: /* 0-4-6-7-6-4-0 */
            t_a = (t0 * 0.5f) + t1 + t2;
            t_b = (t0 * 0.5f) + t2;
            t_c = (t0 * 0.5f);
            break;
        case 2: /* 0-2-6-7-6-2-0 */
            t_a = (t0 * 0.5f) + t1;
            t_b = (t0 * 0.5f) + t1 + t2;
            t_c = (t0 * 0.5f);
            break;
        case 3: /* 0-2-3-7-3-2-0 */
            t_a = (t0 * 0.5f);
            t_b = (t0 * 0.5f) + t1 + t2;
            t_c = (t0 * 0.5f) + t2;
            break;
        case 4: /* 0-1-3-7-3-1-0 */
            t_a = (t0 * 0.5f);
            t_b = (t0 * 0.5f) + t1;
            t_c = (t0 * 0.5f) + t1 + t2;
            break;
        case 5: /* 0-1-5-7-5-1-0 */
            t_a = (t0 * 0.5f) + t2;
            t_b = (t0 * 0.5f);
            t_c = (t0 * 0.5f) + t1 + t2;
            break;
        case 6: /* 0-4-5-7-5-4-0 */
            t_a = (t0 * 0.5f) + t1 + t2;
            t_b = (t0 * 0.5f);
            t_c = (t0 * 0.5f) + t1;
            break;
        default:
            t_a = t_b = t_c = 0.5f;  /* 零矢量：全低电平 */
            break;
    }

    /* 归一化后 t∈[0,1] 直接对应 CCR 比例（占空比 = t） */
    *ta = t_a;
    *tb = t_b;
    *tc = t_c;
}
