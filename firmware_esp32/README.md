# esp32_dashboard — TwinVector ESP32-C3 无线仪表盘

## 依赖库（必须先装）

Arduino IDE → 库管理器（Ctrl+Shift+I）→ 搜索安装：

- **WebSockets** by Markus Sattler (Links2004)

## 编译上传步骤

1. Arduino IDE 打开 `esp32_dashboard.ino`
2. 开发板管理器确认已装 **esp32 by Espressif Systems**
3. 工具 → 开发板 → **ESP32-C3 DevKitM-1**（或 ESP32C3 Dev Module）
4. 选好 COM 口 → 点上传
5. 打开串口监视器（115200）确认打印 AP/WebSocket 信息

## 接线（连 G431）

| G431 | ESP32-C3 | 线 |
|---|---|---|
| PA2 (LPUART1_TX) | GPIO6 (Serial1 RX) | 杜邦线 |
| PA3 (LPUART1_RX) | GPIO7 (Serial1 TX) | 杜邦线 |
| GND | GND | 杜邦线（共地必须） |

## 使用

1. ESP32 上电 → 手机/电脑搜到 WiFi **FOC-Dashboard**（无密码）连上
2. 浏览器打开 `http://192.168.4.1` → 实时仪表盘
3. G431 上电运行 → 仪表盘开始显示 rpm/角度/Vq 并画曲线

## 数据流

```
G431 LPUART1 921600 ──JSON──> ESP32 Serial1 ──WebSocket 81──> 浏览器
```

## 调试提示

- 仪表盘没数据：先看 ESP32 串口监视器——G431 没发数据时只有心跳帧，此时查 G431 侧
- JSON 帧格式：`{"rpm":300,"angle":180,"vq":1500,"temp":0,"st":1}`
- 手机连不上：确认 ESP32 上电后 AP 名出现（串口监视器会打印 IP）
