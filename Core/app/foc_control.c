/*
 * foc_control.c — FOC 速度闭环主控实现
 *
 * 控制链（10kHz 中断）：
 *   速度误差 → PI → Vq → 逆Park（Vd=0）→ SVPWM → TIM1 CCR
 *   电角度 = (机械角 × 极对数 − 对齐偏置)，机械角由主循环缓存
 *
 * 无电流采样（面包板阶段）：电压模式 FOC，Vq 直接控制力矩方向
 * 对齐：上电后施加固定电压 500ms，把转子拉到 θ=0 电角度位置，
 *       记录该位置的机械角为偏置，之后电角度连续跟踪
 */
#include "foc_control.h"
#include "globals.h"
#include "drv8313.h"
#include "clarke_park.h"
#include "svpwm.h"
#include "pi_controller.h"
#include "main.h"

#include <math.h>

#define TWO_PI          (6.28318530718f)
#define SPEED_LP_TAU    0.05f   /* 测速低通时间常数 50ms */
#define ALIGN_VOLTAGE   1.5f    /* 对齐电压（伏特），够拉转子又不烧 */
#define ALIGN_MS        500u    /* 对齐持续时间 */
#define VQ_MAX          6.5f    /* Vq 限幅 ≈ Vdc/√3（11.4V/1.732） */

/* 速度环 PI 参数（10kHz tick）——起调值，实测需调 */
#define PI_SPEED_KP     0.10f
#define PI_SPEED_KI     0.00002f   /* 每 tick 积分系数 ≈ Kp/5000 */
#define PI_SPEED_LIMIT  VQ_MAX
#define PI_SPEED_INTLIM 1.0f

/* 内部状态 */
static foc_state_t foc_state = FOC_STATE_IDLE;
static pi_controller_t pi_speed;
static float offset_elec = 0.0f;   /* 对齐后电角度偏置 */
static volatile float theta_elec = 0.0f;

/* 测速中间量（主循环使用） */
static uint32_t last_tick = 0;
static float last_mech_rad = 0.0f;
static float speed_lp = 0.0f;

/* 对齐计时 */
static uint32_t align_start_ms = 0;

void foc_init(void)
{
    pi_init(&pi_speed, PI_SPEED_KP, PI_SPEED_KI, PI_SPEED_LIMIT, PI_SPEED_INTLIM);
    foc_state = FOC_STATE_IDLE;
    foc_speed_rpm = 0.0f;
    foc_vq_out = 0.0f;
    foc_running = 0;
    target_speed_rpm = 0.0f;
    drv8313_off();
}

void foc_start(void)
{
    if (foc_state == FOC_STATE_IDLE) {
        foc_state = FOC_STATE_ALIGN;
        align_start_ms = HAL_GetTick();
    }
}

void foc_stop(void)
{
    foc_state = FOC_STATE_IDLE;
    foc_running = 0;
    target_speed_rpm = 0.0f;
    drv8313_off();
}

void foc_set_target(float rpm)
{
    target_speed_rpm = rpm;
}

foc_state_t foc_get_state(void)
{
    return foc_state;
}

/* ── 主循环（非实时）：编码器测速 + 低通 + 状态机推进 ── */
void foc_update(void)
{
    /* 测速：用 tick 计数差求 dt，避免主循环周期不定导致速度计算失真 */
    uint32_t now_tick = foc_tick_count;
    uint32_t dt_ticks = now_tick - last_tick;   /* uint32 环绕安全 */
    if (dt_ticks < 50) {
        return;   /* 至少 5ms 采样一次 */
    }
    last_tick = now_tick;

    float dt = (float)dt_ticks * 0.0001f;       /* 10kHz tick → 秒 */
    float mech_rad = (float)encoder_raw * TWO_PI / (float)FOC_ENCODER_RES;

    /* 角度差分 + 环绕修正（±π 内取最短路径） */
    float delta = mech_rad - last_mech_rad;
    if (delta > 3.14159265f) delta -= TWO_PI;
    if (delta < -3.14159265f) delta += TWO_PI;
    last_mech_rad = mech_rad;

    float speed_rad_s = delta / dt;
    float speed_rpm_raw = speed_rad_s * 60.0f / TWO_PI;

    /* 一阶低通：α = dt/(τ+dt)，τ=50ms */
    float alpha = dt / (SPEED_LP_TAU + dt);
    speed_lp += alpha * (speed_rpm_raw - speed_lp);
    foc_speed_rpm = speed_lp;

    /* 状态机推进：对齐 500ms 后记录偏置，切入闭环 */
    if (foc_state == FOC_STATE_ALIGN) {
        if (HAL_GetTick() - align_start_ms >= ALIGN_MS) {
            offset_elec = mech_rad * (float)FOC_POLE_PAIRS;
            pi_reset(&pi_speed);
            foc_state = FOC_STATE_CLOSED_LOOP;
            foc_running = 1;
        }
    }
}

/* ── TIM1 更新中断（10kHz，实时）：控制链 ── */
void foc_isr(void)
{
    float vq, valpha, vbeta, ta, tb, tc;

    foc_tick_count++;

    if (foc_state == FOC_STATE_IDLE) {
        drv8313_set_duty(0.0f, 0.0f, 0.0f);
        return;
    }

    if (foc_state == FOC_STATE_ALIGN) {
        /* 对齐：电角度固定在 0，施加 Vq 把转子拉到 θ=0 */
        theta_elec = 0.0f;
        valpha = 0.0f;
        vbeta = ALIGN_VOLTAGE;
        svpwm_calc(valpha, vbeta, 11.4f, &ta, &tb, &tc);
        drv8313_set_duty(ta, tb, tc);
        return;
    }

    /* CLOSED_LOOP：速度 PI → Vq，Vd=0 电压模式 */
    float err = target_speed_rpm - foc_speed_rpm;
    vq = pi_update(&pi_speed, err);
    foc_vq_out = vq;

    /* 电角度：机械角 × 极对数 − 对齐偏置 */
    float mech_rad = (float)encoder_raw * TWO_PI / (float)FOC_ENCODER_RES;
    theta_elec = mech_rad * (float)FOC_POLE_PAIRS - offset_elec;

    /* 逆 Park（Vd=0）→ SVPWM → 写占空比 */
    inv_park_transform(0.0f, vq, theta_elec, &valpha, &vbeta);
    svpwm_calc(valpha, vbeta, 11.4f, &ta, &tb, &tc);
    drv8313_set_duty(ta, tb, tc);
}
