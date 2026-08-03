/*
 * clarke_park.c — FOC 坐标变换实现
 * 常量 1/sqrt(3) ≈ 0.57735026919，编译期常量避免运行时除法
 */
#include "clarke_park.h"
#include <math.h>

static const float INV_SQRT3 = 0.57735026919f;

void clarke_transform(float ia, float ib, float *ialpha, float *ibeta)
{
    /* 第三相由 KCL 推出：ic = -(ia+ib)，无需采样 */
    *ialpha = ia;
    *ibeta  = (ia + 2.0f * ib) * INV_SQRT3;
}

void park_transform(float ialpha, float ibeta, float theta, float *id, float *iq)
{
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);

    *id =  ialpha * cos_t + ibeta * sin_t;
    *iq = -ialpha * sin_t + ibeta * cos_t;
}

void inv_park_transform(float vd, float vq, float theta, float *valpha, float *vbeta)
{
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);

    *valpha = vd * cos_t - vq * sin_t;
    *vbeta  = vd * sin_t + vq * cos_t;
}
