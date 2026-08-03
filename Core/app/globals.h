/*
 * globals.h — 全局变量集中声明
 * 标注规则：每个变量注明【谁写/谁读】
 *
 * 线程模型：
 *   FOC 10kHz 中断（TIM1 更新中断）→ 写 foc_* 变量
 *   主循环 → 读 foc_* 变量，写 encoder_* 变量
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

/* ── 编码器角度（主循环写 / FOC 中断读） ── */
extern volatile float encoder_angle_deg;    /* 主循环读 AS5600 更新（0~360°） */
extern volatile uint16_t encoder_raw;       /* 主循环读 AS5600 原始值（0~4095） */

/* ── FOC 状态（FOC 中断写 / 主循环读） ── */
extern volatile float foc_speed_rpm;        /* FOC 中断计算的实际转速 */
extern volatile float foc_vq_out;           /* FOC 中断输出的 Vq（伏特） */
extern volatile uint8_t foc_running;        /* 1=闭环运行中，0=停机 */

/* ── 控制目标（主循环写 / FOC 中断读） ── */
extern volatile float target_speed_rpm;     /* 目标转速 */

/* ── 节拍计数 ── */
extern volatile uint32_t foc_tick_count;    /* FOC 中断次数（10kHz 递增） */

#endif /* GLOBALS_H */
