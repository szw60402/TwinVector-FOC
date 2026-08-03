/*
 * foc_control.h — FOC 主控（速度闭环，电压模式）
 *
 * 架构：
 *   TIM1 更新中断 @10kHz（RCR=1）→ foc_isr()（实时部分）
 *   主循环 → foc_update()（非实时部分：编码器测速+低通）
 *
 * 状态机：
 *   IDLE（停机）→ ALIGN（对齐 500ms）→ CLOSED_LOOP（速度闭环）
 *
 * 2205 电机参数（默认，可在 foc_init 里改）：
 *   极对数 7，编码器 12 位
 */
#ifndef FOC_CONTROL_H
#define FOC_CONTROL_H

#include <stdint.h>

/* 极对数与编码器分辨率 */
#define FOC_POLE_PAIRS       7
#define FOC_ENCODER_RES      4096

/* 状态机 */
typedef enum {
    FOC_STATE_IDLE = 0,
    FOC_STATE_ALIGN,
    FOC_STATE_CLOSED_LOOP,
} foc_state_t;

/* 初始化：清零 PI、设定参数、对齐前状态 */
void foc_init(void);

/* 主循环调用（非实时）：编码器角度 → 差分测速 → 低通滤波 */
void foc_update(void);

/* TIM1 更新中断调用（10kHz 实时）：Clarke/Park/PI/逆Park/SVPWM */
void foc_isr(void);

/* 启动（IDLE→ALIGN）。停止则 foc_stop() */
void foc_start(void);

/* 停止：输出关断，回到 IDLE */
void foc_stop(void);

/* 设定目标转速（rpm，可正可负） */
void foc_set_target(float rpm);

/* 查询状态 */
foc_state_t foc_get_state(void);

#endif /* FOC_CONTROL_H */
