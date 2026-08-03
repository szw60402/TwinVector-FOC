/*
 * dashboard.h — ESP32 仪表盘数据上报（LPUART1，921600bps）
 * 帧格式（JSON，20ms 一帧）：
 *   {"rpm":1234,"angle":180,"vq":2500,"temp":25,"st":1}\r\n
 *   单位：rpm 整数 / angle 整数度 / vq 毫伏 / temp 摄氏度(预留=0)
 * ESP32 端按行解析 JSON 转发 WebSocket
 */
#ifndef DASHBOARD_H
#define DASHBOARD_H

/* 初始化：无（LPUART1 已由 CubeMX 配置） */
void dashboard_init(void);

/* 主循环调用：内部 20ms 节拍，到点发一帧 */
void dashboard_update(void);

/* 立即发送一帧（调试用） */
void dashboard_send_now(void);

#endif /* DASHBOARD_H */
