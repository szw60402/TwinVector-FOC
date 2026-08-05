# TwinVector

> 双 MCU 实时 FOC 电机控制系统 · STM32G431 + ESP32-C3 · 无线蓝牙透传

## 项目概述

**TwinVector** 是一个在面包板上从零搭建的双 MCU FOC（磁场定向控制）电机控制系统。STM32G431CBU6 在 10kHz 中断内实时完成速度闭环控制链，ESP32-C3 通过 BLE 将转速/角度/电压实时推送到手机 App。全代码手写，无第三方算法库。

> 核心亮点：电压模式 FOC 全链路 · 双 MCU 实时/非实时解耦 · 无线 BLE 数据透传

---

## 技术栈

| 层面 | 技术 |
|------|------|
| **FOC 主控** | STM32G431CBU6 (Cortex-M4) @ 170MHz |
| **功率驱动** | DRV8313 3-PWM 模式，20kHz PWM |
| **角度反馈** | AS5600 磁编码器，12 位 I2C @ 100kHz |
| **通信** | LPUART1 @ 921600bps |
| **仪表盘** | ESP32-C3 BLE UART 无线透传（NimBLE）→ 手机 App |
| **开发环境** | CubeMX 6.x + CMake + arm-none-eabi-gcc / Arduino IDE |

---

## 硬件

| 组件 | 规格 | 数量 |
|------|------|:--:|
| 主控 | STM32G431CBU6 核心板 (Type-C / ST-Link V3) | 1 |
| 功率驱动 | DRV8313 三相驱动模块 (3-PWM) | 1 |
| 电机 | RS2205 无刷电机 (3mm 轴, 7 对极) | 1 |
| 编码器 | AS5600 磁编码器模块 + 径向磁环 | 1 |
| 仪表盘 | ESP32-C3 DevKitM-1 | 1 |
| 电源 | 12V 2A 适配器 + HW-131 面包板电源模块 | 1 |

### 接线定义（核心板引脚向外发散）

**G431 引脚 → 外设**

| G431 引脚 | 功能 | 连到哪 |
|:---------:|:----:|------|
| PA8 / PA9 / PA10 | TIM1_CH1/2/3 @20kHz | DRV8313 IN1/IN2/IN3 |
| PA15 | I2C1_SCL | AS5600 SCL + 4.7kΩ 上拉 |
| PB7 | I2C1_SDA | AS5600 SDA + 4.7kΩ 上拉 |
| PA2 / PA3 | LPUART1 TX/RX @921600 | ESP32 GPIO6 / GPIO7 |
| PC13 | LED | 板载心跳灯 |

**电源双轨**：上红轨 11.4V → 仅 DRV8313 VM；下红轨 3.3V → AS5600 / 上拉 / EN

---

## 软件架构

**STM32G431 端（10kHz 实时）**

```
TIM1 更新中断 @10kHz（RCR=1）
  ├─ 速度误差 → PI（抗积分饱和）
  ├─ Vq 电压指令（电压模式 FOC，Vd=0）
  ├─ 逆 Park → SVPWM（扇区法 + 七段式 + 过调制保护）
  └─ TIM1 CCR 写入
主循环（非实时）
  ├─ I2C 轮询 AS5600（0x0C/0x0D → 12 位角度）
  ├─ 差分测速 + 一阶低通（tick 差求 dt）
  └─ 20ms 一帧 JSON → LPUART → ESP32
```

**ESP32-C3 端（无线透传）**

- BLE 广播 `FOC-BT`（Nordic UART Service，标准协议）
- Serial1 @921600 收 JSON → 蓝牙透传（手机 App 按行显示）
- 手机 App：Serial Bluetooth Terminal（安卓）/ BlueSPP（苹果）

---

## 项目结构

```
TwinVector-FOC/
├── Core/                # STM32G431 固件
│   ├── app/             # 应用层：foc_control.c / dashboard.c
│   ├── drivers/         # 驱动层：as5600.c / drv8313.c
│   ├── utils/           # 算法层：clarke_park.c / svpwm.c / pi_controller.c
│   └── Src/ Inc/        # CubeMX 生成区
├── firmware_esp32/      # ESP32 仪表盘固件 (.ino)
├── TwinVector.ioc       # CubeMX 工程
├── AGENTS.md            # 开发规范（分层/中断安全/代码风格）
└── CMakeLists.txt       # CMake 构建（显式源文件列表）
```

---

## 构建与烧录

**G431（CMake）**
```bash
cmake --preset Debug
cmake --build build/Debug
# 产物：build/Debug/TwinVector.elf
```
CubeIDE 打开 `TwinVector.ioc` 也可直接编译调试（ST-Link V3 一线烧录）。

**ESP32（Arduino IDE）**
1. 打开 `firmware_esp32/firmware_esp32.ino`
2. 依赖库：NimBLE-Arduino by h2zero
3. 选 ESP32C3 Dev Module → 上传

---

## 使用

1. G431 上电：自动执行 对齐(500ms) → 速度闭环(默认 300rpm)
2. 手机装 Serial Bluetooth Terminal，扫描连接 BLE 设备 `FOC-BT`
3. App 里实时滚动 `{"rpm":300,"angle":180,"vq":1500,"temp":0,"st":2}` 数据帧

数据帧格式：`{"rpm":300,"angle":180,"vq":1500,"temp":0,"st":2}`（st: 0=停机 1=对齐 2=闭环）

---

## 踩坑记录

- **SVPWM 量纲错误**：电压未除母线电压归一化 → 会输出满占空比，归一化后修复（量纲检查的重要性）
- **LPUART1 时钟源**：921600 需显式选 PCLK1（新版 CubeMX 时钟树里配置）
- **I2C1 引脚**：G431CBU6 的 I2C1_SCL 实际分配 PA15（非 PB6/PB15），以 CubeMX 生成为准
- **电流采样**：面包板阶段 DRV8313 简化模块 PGND 未引出 → 电压模式 FOC，电流环留 PCB 阶段

## License

MIT

## 调试提示

- 电机转向相反 → 交换任意两根电机相线（OUT1/OUT2 对调）
- 手机无数据 → 先看 G431 串口是否有 JSON 帧，再查 ESP32 串口日志（BLE 是否被连上）
- 编码器角度不变化 → 检查磁环径向充磁 + AS5600 与磁环间距 ≥1mm
