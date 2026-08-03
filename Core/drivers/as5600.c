/*
 * as5600.c — AS5600 I2C 轮询读取实现
 * I2C 句柄 hi2c1 由 CubeMX 生成（PA15/PB7，100kHz）
 * 超时 50ms：AS5600 可能忙，失败直接返回 0 不重试（主循环下一轮再读）
 */
#include "as5600.h"
#include "main.h"

/* CubeMX 生成的 I2C1 句柄 */
extern I2C_HandleTypeDef hi2c1;

void as5600_init(void)
{
    /* 硬件已就绪（上拉电阻 + 3.3V 供电），无需软件初始化 */
}

uint8_t as5600_read_raw(uint16_t *raw)
{
    uint8_t buf[2];

    /* 读 0x0C/0x0D 两个字节：0x0C 高字节、0x0D 低字节（12 位大端） */
    if (HAL_I2C_Mem_Read(&hi2c1, (AS5600_ADDR << 1), AS5600_REG_RAW_ANGLE,
                         I2C_MEMADD_SIZE_8BIT, buf, 2, 50) != HAL_OK) {
        return 0;
    }

    *raw = ((uint16_t)buf[0] << 8) | buf[1];
    *raw &= 0x0FFF;  /* 12 位 */
    return 1;
}

float as5600_read_deg(void)
{
    uint16_t raw;

    if (!as5600_read_raw(&raw)) {
        return 0.0f;
    }
    return (float)raw * 360.0f / (float)AS5600_RESOLUTION;
}
