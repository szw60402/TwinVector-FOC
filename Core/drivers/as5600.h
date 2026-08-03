/*
 * as5600.h — AS5600 磁编码器驱动（I2C1，地址 0x36）
 * 硬件接线：VCC→3.3V轨, GND→GND轨, SCL→PA15, SDA→PB7
 *           SCL/SDA 各有 4.7kΩ 上拉到 3.3V
 *
 * 注意：
 * - 主循环轮询读取（I2C 禁止进 FOC 10kHz 中断）
 * - 寄存器 0x0C/0x0D 为原始 12 位角度（0~4095）
 * - 磁环必须径向充磁，读不到变化先查磁环
 */
#ifndef AS5600_H
#define AS5600_H

#include <stdint.h>

#define AS5600_ADDR         0x36   /* 7 位地址 */
#define AS5600_REG_RAW_ANGLE 0x0C  /* 原始角度，2 字节大端 */
#define AS5600_RESOLUTION   4096   /* 12 位 */

/* 初始化：无特殊配置，7 位地址 + 4.7kΩ 上拉已由硬件完成 */
void as5600_init(void);

/* 读取原始 12 位角度（0~4095）。成功返回 1，失败返回 0 */
uint8_t as5600_read_raw(uint16_t *raw);

/* 读取角度（度，0~360）。失败返回 0 */
float as5600_read_deg(void);

#endif /* AS5600_H */
