# firmware_esp32 — TwinVector ESP32-C3 BLE 仪表盘

ESP32-C3 通过 BLE 把 G431 的 JSON 数据转发到手机，负责非实时的人机界面。

## 依赖库

Arduino IDE → 库管理器 → 安装：

- **NimBLE-Arduino** by h2zero

## 编译上传

1. Arduino IDE 打开 `firmware_esp32.ino`
2. 开发板选择 **ESP32C3 Dev Module**
3. 选好串口 → 上传

## 接线（连 G431）

| G431 | ESP32-C3 | 说明 |
|---|---|---|
| PA2 (LPUART1_TX) | GPIO6 (Serial1 RX) | 杜邦线 |
| PA3 (LPUART1_RX) | GPIO7 (Serial1 TX) | 杜邦线 |
| GND | GND | 共地必须 |

波特率 921600。

## 使用

1. 手机安装 Serial Bluetooth Terminal（安卓）或 BlueSPP（苹果）
2. 扫描并连接 BLE 设备 **FOC-BT**
3. G431 上电运行后，App 按行显示 JSON 数据

## 数据流

```
G431 LPUART1 921600 ──JSON──> ESP32 Serial1 ──BLE (NUS)──> 手机 App
```

## 数据帧

```json
{"rpm":300,"angle":180,"vq":1500,"temp":0,"st":2}
```

`st`：0=停机，1=对齐，2=闭环。

## 注意

- ESP32-C3 同一时间只能开 WiFi 或蓝牙，本固件只开蓝牙。
- 断开连接后固件会自动重新广播，手机可以再次连接。