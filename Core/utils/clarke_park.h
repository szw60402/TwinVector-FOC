/*
 * clarke_park.h — FOC 坐标变换（Clarke/Park + 逆 Park）
 * 硬件相关：无（纯数学，单精度 float，Cortex-M4 硬件 FPU）
 *
 * 公式：
 *   Clarke:  ia + ib + ic = 0 → ic = -(ia+ib)
 *            ialpha = ia, ibeta = (ia + 2*ib)/sqrt(3)
 *   Park:    id = ialpha*cosθ + ibeta*sinθ
 *            iq = -ialpha*sinθ + ibeta*cosθ
 *   invPark: valpha = vd*cosθ - vq*sinθ
 *            vbeta = vd*sinθ + vq*cosθ
 */
#ifndef CLARKE_PARK_H
#define CLARKE_PARK_H

#include <stdint.h>

/* Clarke 变换：两相电流 -> αβ 静止坐标系 */
void clarke_transform(float ia, float ib, float *ialpha, float *ibeta);

/* Park 变换：αβ -> dq 旋转坐标系（θ 为电角度，弧度） */
void park_transform(float ialpha, float ibeta, float theta, float *id, float *iq);

/* 逆 Park 变换：dq -> αβ（θ 为电角度，弧度） */
void inv_park_transform(float vd, float vq, float theta, float *valpha, float *vbeta);

#endif /* CLARKE_PARK_H */
