/*
 * esp32_dashboard.ino — TwinVector ESP32-C3 无线仪表盘
 * 功能：AP 热点 + UART 收 G431 JSON + WebSocket 推送浏览器
 *
 * 硬件接线（与 G431）：
 *   G431 PA2(TX)  → ESP32 GPIO6(RX)   [Serial1 RX]
 *   G431 PA3(RX)  ← ESP32 GPIO7(TX)   [Serial1 TX]
 *   G431 GND      → ESP32 GND         （共地必须）
 *   波特率 921600
 *
 * 使用：
 *   手机连 WiFi "FOC-Dashboard"（无密码）
 *   浏览器打开 http://192.168.4.1  → 实时仪表盘
 *
 * 依赖库（Arduino 库管理器安装）：
 *   WebSockets  by Markus Sattler (Links2004)
 */

#include <WiFi.h>
#include <WebSocketsServer.h>

/* ── WiFi AP 配置 ── */
const char *AP_SSID = "FOC-Dashboard";
const char *AP_PASS = NULL;                 /* NULL = 开放网络，免密码 */
IPAddress AP_IP(192, 168, 4, 1);
IPAddress AP_GW(192, 168, 4, 1);
IPAddress AP_MASK(255, 255, 255, 0);

/* ── WebSocket 服务器（端口 81） ── */
WebSocketsServer webSocket(81);

/* ── UART 接收（Serial1 → GPIO6/GPIO7） ── */
#define UART_BAUD   921600
#define RX_PIN      6
#define TX_PIN      7
#define LINE_MAX    128          /* JSON 帧最长 128 字节 */

static char line_buf[LINE_MAX];
static uint8_t line_len = 0;

/* ── 内嵌网页（手机浏览器访问根路径时返回） ── */
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FOC Dashboard</title>
<style>
  body { margin:0; font-family:system-ui,sans-serif; background:#0f1420; color:#e8ecf4;
         display:flex; flex-direction:column; align-items:center; min-height:100vh; }
  h1 { font-size:20px; margin:18px 0 4px; letter-spacing:2px; color:#8ab4ff; }
  .rpm-box { background:#161d2e; border:1px solid #2a3550; border-radius:16px;
             width:min(92vw,420px); padding:18px; margin:14px 0; text-align:center; }
  #rpm { font-size:64px; font-weight:700; color:#4ade80; }
  .unit { font-size:16px; color:#64748b; }
  .row { display:flex; gap:10px; width:min(92vw,420px); }
  .card { flex:1; background:#161d2e; border:1px solid #2a3550; border-radius:12px;
          padding:10px; text-align:center; }
  .card .v { font-size:20px; font-weight:600; color:#e8ecf4; }
  .card .l { font-size:11px; color:#64748b; margin-top:2px; }
  #st { font-size:13px; padding:3px 10px; border-radius:99px; display:inline-block; }
  .run  { background:#166534; color:#86efac; }
  .idle { background:#4b1d1d; color:#fca5a5; }
  .align{ background:#7c3a1d; color:#fdba74; }
  canvas { width:min(92vw,420px); height:140px; background:#101624; border:1px solid #2a3550;
           border-radius:12px; margin-top:10px; }
  .conn { font-size:12px; color:#64748b; margin:8px; }
  .ok { color:#4ade80; } .bad { color:#f87171; }
</style>
</head>
<body>
  <h1>TWINVECTOR · FOC</h1>
  <div class="rpm-box">
    <div id="rpm">0</div>
    <div class="unit">RPM</div>
  </div>
  <div class="row">
    <div class="card"><div class="v" id="angle">0</div><div class="l">角度 °</div></div>
    <div class="card"><div class="v" id="vq">0</div><div class="l">Vq mV</div></div>
    <div class="card"><div class="v" id="temp">0</div><div class="l">温度 ℃</div></div>
  </div>
  <div style="margin:12px 0"><span id="st" class="idle">等待数据</span></div>
  <canvas id="plot"></canvas>
  <div class="conn" id="conn">连接中…</div>

<script>
var ws = new WebSocket('ws://' + location.hostname + ':81');
var hist = [];                 /* rpm 历史曲线 */
var MAX = 200;

ws.onopen = function(){ document.getElementById('conn').innerHTML =
  '<span class="ok">● 已连接</span>'; };
ws.onclose = function(){ document.getElementById('conn').innerHTML =
  '<span class="bad">● 已断开，重连中…</span>';
  setTimeout(function(){ ws = new WebSocket('ws://' + location.hostname + ':81'); }, 2000); };
ws.onmessage = function(ev){
  try {
    var d = JSON.parse(ev.data);
    if (d.beat) return;          /* 心跳帧：忽略，不刷新页面 */
    document.getElementById('rpm').textContent = d.rpm;
    document.getElementById('angle').textContent = d.angle;
    document.getElementById('vq').textContent = d.vq;
    document.getElementById('temp').textContent = d.temp;
    var st = document.getElementById('st');
    if (d.st == 2) { st.className = 'run';  st.textContent = '闭环运行'; }
    else if (d.st == 1) { st.className = 'align'; st.textContent = '对齐中'; }
    else { st.className = 'idle'; st.textContent = '停机'; }
    hist.push(d.rpm);
    if (hist.length > MAX) hist.shift();
    draw();
  } catch(e) {}
};

function draw(){
  var c = document.getElementById('plot'), ctx = c.getContext('2d');
  var W = c.width = c.offsetWidth * 2, H = c.height = c.offsetHeight * 2;
  ctx.clearRect(0,0,W,H);
  ctx.strokeStyle = '#334155'; ctx.beginPath();
  ctx.moveTo(0,H/2); ctx.lineTo(W,H/2); ctx.stroke();
  if (hist.length < 2) return;
  var max = 1000, min = -1000;
  hist.forEach(function(v){ if(v>max)max=v; if(v<min)min=v; });
  var range = (max-min) || 1;
  ctx.strokeStyle = '#4ade80'; ctx.lineWidth = 2; ctx.beginPath();
  for (var i=0;i<hist.length;i++){
    var x = i/(MAX-1)*W, y = H/2 - (hist[i]-min)/range*H*0.9;
    i ? ctx.lineTo(x,y) : ctx.moveTo(x,y);
  }
  ctx.stroke();
}
window.onresize = draw;
</script>
</body>
</html>
)rawliteral";

/* ── WebSocket 事件回调 ── */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type) {
        case WStype_CONNECTED:
            Serial.printf("WS client #%u connected\n", num);
            break;
        case WStype_DISCONNECTED:
            Serial.printf("WS client #%u disconnected\n", num);
            break;
        default:
            break;   /* 本项目只推数据，不收客户端消息 */
    }
}

/* ── 处理一帧 G431 数据（去掉行尾）并广播 ── */
void broadcast_line(void)
{
    if (line_len == 0) return;

    /* 去掉 \r 和 \n */
    while (line_len > 0 &&
           (line_buf[line_len-1] == '\r' || line_buf[line_len-1] == '\n')) {
        line_len--;
    }
    if (line_len == 0) return;

    webSocket.broadcastTXT((uint8_t *)line_buf, line_len);
    line_len = 0;
}

void setup()
{
    Serial.begin(115200);   /* USB 调试口（烧录也用这个） */

    /* AP 模式：开放热点，IP 192.168.4.1 */
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(100);
    Serial.printf("AP: %s @ %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

    /* WebSocket 服务器 */
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("WebSocket: ws://192.168.4.1:81");

    /* 与 G431 的串口（GPIO6=RX, GPIO7=TX, 921600） */
    Serial1.begin(UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("UART1: 921600 @ RX=6 TX=7");
}

void loop()
{
    webSocket.loop();

    /* 逐字节收 G431 帧，遇到换行符广播 */
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

    /* 200ms 心跳：广播一条 keepalive（浏览器断线检测用） */
    static uint32_t last_beat = 0;
    if (millis() - last_beat > 200) {
        last_beat = millis();
        if (webSocket.connectedClients() > 0) {
            webSocket.broadcastTXT("{\"beat\":1}");
        }
    }
}
