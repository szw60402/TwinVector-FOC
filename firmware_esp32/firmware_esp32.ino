/*
 * firmware_esp32.ino — TwinVector ESP32-C3 蓝牙仪表盘
 * 功能：BLE UART 透传 + UART 收 G431 JSON + 推送手机
 *
 * 硬件接线（与 G431）：
 *   G431 PA2(TX)  → ESP32 GPIO6(RX)   [Serial1 RX]
 *   G431 PA3(RX)  ← ESP32 GPIO7(TX)   [Serial1 TX]
 *   G431 GND      → ESP32 GND         （共地必须）
 *   波特率 921600
 *
 * 使用：
 *   手机装 "Serial Bluetooth Terminal"（安卓）/ "BlueSPP"（苹果）
 *   打开 App → 扫描连接 BLE 设备 "FOC-BT" → 实时滚动 JSON 数据
 *   数据格式：{"rpm":300,"angle":180,"vq":1500,"temp":0,"st":2}
 *
 * 注意：
 *   ESP32-C3 同一时间只能 WiFi 或蓝牙，此固件纯蓝牙，无 WiFi。
 *   依赖库：NimBLE-Arduino (h2zero)，Arduino 库管理器搜索安装。
 */

#include "NimBLEDevice.h"

/* ── BLE UART 服务（Nordic UART Service，Serial Bluetooth Terminal 通用） ── */
#define BT_NAME        "FOC-BT"
#define NUS_SERVICE    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_CHAR    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  /* 通知：ESP32→手机 */
#define NUS_RX_CHAR    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  /* 写入：手机→ESP32 */

NimBLEServer      *pServer = NULL;
NimBLECharacteristic *pTxChar = NULL;
static bool bt_connected = false;

/* ── BLE 连接回调 ── */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *server, NimBLEConnInfo &connInfo) override {
        bt_connected = true;
        Serial.printf("BLE: 手机已连接\n");
    }
    void onDisconnect(NimBLEServer *server, NimBLEConnInfo &connInfo, int reason) override {
        bt_connected = false;
        Serial.printf("BLE: 手机断开，重新广播\n");
        NimBLEDevice::startAdvertising();   /* 断开后必须重新广播，手机才能再连 */
    }
};

/* ── UART 接收（Serial1 → GPIO6/GPIO7） ── */
#define UART_BAUD   921600
#define RX_PIN      6
#define TX_PIN      7
#define LINE_MAX    128          /* JSON 帧最长 128 字节 */

static char line_buf[LINE_MAX];
static uint8_t line_len = 0;

/* ── 通过蓝牙通知发一帧数据 ── */
void bt_send(const char *data, size_t len)
{
    if (!bt_connected || len == 0) return;
    pTxChar->setValue((const uint8_t *)data, len);
    pTxChar->notify();
}

/* ── 处理一帧 G431 数据（去掉行尾）并蓝牙转发 ── */
void broadcast_line(void)
{
    if (line_len == 0) return;

    /* 去掉 \r 和 \n */
    while (line_len > 0 &&
           (line_buf[line_len-1] == '\r' || line_buf[line_len-1] == '\n')) {
        line_len--;
    }
    if (line_len == 0) return;

    size_t len = line_len;
    char *data = line_buf;
    data[len++] = '\n';          /* 补上行尾，App 按行显示 */
    bt_send(data, len);
    line_len = 0;
}

void setup()
{
    Serial.begin(115200);   /* USB 调试口（烧录也用这个） */

    /* ── BLE 服务初始化 ── */
    NimBLEDevice::init(BT_NAME);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(NUS_SERVICE);
    pTxChar = pService->createCharacteristic(
                  NUS_TX_CHAR, NIMBLE_PROPERTY::NOTIFY);
    pService->createCharacteristic(
                  NUS_RX_CHAR, NIMBLE_PROPERTY::WRITE);

    pService->start();
    NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(NUS_SERVICE);
    NimBLEDevice::startAdvertising();
    Serial.printf("BLE: %s 广播中\n", BT_NAME);

    /* 与 G431 的串口（GPIO6=RX, GPIO7=TX, 921600） */
    Serial1.begin(UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("UART1: 921600 @ RX=6 TX=7");
}

void loop()
{
    /* 逐字节收 G431 帧，遇到换行符蓝牙转发 */
    while (Serial1.available() > 0) {
        char c = (char)Serial1.read();

        if (c == '\n') {
            broadcast_line();
        } else if (line_len < LINE_MAX - 1) {
            line_buf[line_len++] = c;
        } else {
            line_len = 0;   /* 超长帧丢弃，防止缓冲溢出 */
        }
    }
}