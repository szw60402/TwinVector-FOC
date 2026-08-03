/*
 * dashboard.c — JSON 帧发送实现
 * 手写 int 转字符串，避免 snprintf 浮点开销与栈占用
 * 发送失败（ESP32 未连）静默跳过，绝不阻塞 FOC
 */
#include "dashboard.h"
#include "globals.h"
#include "foc_control.h"
#include "main.h"

#include <string.h>

#define DASHBOARD_PERIOD_MS  20u
#define TX_TIMEOUT_MS        100u

extern UART_HandleTypeDef hlpuart1;

static uint32_t last_send_ms = 0;
static char tx_buf[96];

/* 无符号整数转字符串（手写，10 进制） */
static void u32_to_str(uint32_t val, char *out)
{
    char tmp[12];
    int i = 0;

    do {
        tmp[i++] = (char)('0' + val % 10);
        val /= 10;
    } while (val > 0);

    int j = 0;
    while (i > 0) {
        out[j++] = tmp[--i];
    }
    out[j] = '\0';
}

/* 带符号整数转字符串（负号 + u32_to_str） */
static void i32_to_str(int32_t val, char *out)
{
    if (val < 0) {
        out[0] = '-';
        u32_to_str((uint32_t)(-val), &out[1]);
    } else {
        u32_to_str((uint32_t)val, out);
    }
}

void dashboard_init(void)
{
    last_send_ms = HAL_GetTick();
}

void dashboard_send_now(void)
{
    char s_rpm[12], s_angle[12], s_vq[12];
    int32_t rpm, vq;
    uint32_t angle;

    rpm   = (int32_t)(foc_speed_rpm + 0.5f);
    angle = (uint32_t)(encoder_angle_deg + 0.5f);
    vq    = (int32_t)(foc_vq_out * 1000.0f + 0.5f);   /* V → mV */

    i32_to_str(rpm, s_rpm);
    u32_to_str(angle, s_angle);
    i32_to_str(vq, s_vq);

    /* 组装 JSON（temp 预留 0，STM32 内部温度传感器未启用） */
    size_t n = 0;
    const char *p1 = "{\"rpm\":";
    const char *p2 = ",\"angle\":";
    const char *p3 = ",\"vq\":";
    const char *p4 = ",\"temp\":0,\"st\":";
    const char *p5 = "}\r\n";

    while (*p1) tx_buf[n++] = *p1++;
    for (char *c = s_rpm; *c; c++) tx_buf[n++] = *c;
    while (*p2) tx_buf[n++] = *p2++;
    for (char *c = s_angle; *c; c++) tx_buf[n++] = *c;
    while (*p3) tx_buf[n++] = *p3++;
    for (char *c = s_vq; *c; c++) tx_buf[n++] = *c;
    while (*p4) tx_buf[n++] = *p4++;
    tx_buf[n++] = (char)('0' + (int)foc_get_state());
    while (*p5) tx_buf[n++] = *p5++;

    /* 轮询发送，超时 100ms 失败静默跳过（G4 LPUART 复用 HAL_UART_Transmit） */
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)tx_buf, (uint16_t)n, TX_TIMEOUT_MS);
}

void dashboard_update(void)
{
    uint32_t now = HAL_GetTick();

    if (now - last_send_ms >= DASHBOARD_PERIOD_MS) {
        last_send_ms = now;
        dashboard_send_now();
    }
}
