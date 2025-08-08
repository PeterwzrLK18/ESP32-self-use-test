/*
ESP32-S3 板载 WS2812 控制（1 像素）
行为：
  - 默认：熄灭
  - GPIO4 接地：红灯常亮
  - GPIO5 接地：蓝灯闪烁（非阻塞）

实现要点：
  - WS2812 用 Adafruit NeoPixel 驱动（GRB / 800kHz）
  - LED_PIN 默认为 48（多数 ESP32-S3 DevKit 板载 RGB 的 DIN）
  - 输入脚使用 INPUT_PULLUP，不接线=未触发（HIGH）
  - 仅在“模式变化”时下发指令和打印串口；蓝灯进入即点亮一次
  - 从触发→松开的边沿，进入 OFF_HOLD_MS 的“强制清屏窗口”，避免残留亮
  - 串口打印有 MIN_PRINT_GAP 最小间隔，减少插拔抖动造成的多次打印

可调参数：
  - BLINK_INTERVAL  : 蓝灯闪烁周期（ms）
  - OFF_HOLD_MS     : 释放后强制清屏时长（ms）
  - MIN_PRINT_GAP   : 串口最小打印间隔（ms）
*/

#include <Adafruit_NeoPixel.h>

#define LED_PIN     48
#define NUM_PIXELS  1
#define BTN_RED     4      // 接地=红常亮
#define BTN_BLUE    5      // 接地=蓝闪烁

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

const unsigned long BLINK_INTERVAL = 400;
unsigned long lastToggle = 0;
bool blueOn = false;

// 松开后强制清屏窗口
const unsigned long OFF_HOLD_MS = 300;
unsigned long offHoldUntil = 0;

// 仅用于打印的最小间隔
const unsigned long MIN_PRINT_GAP = 150;
unsigned long lastPrintTs = 0;
int lastPrintedMode = -1;

enum Mode { OFF, RED_ON, BLUE_BLINK };
Mode mode = OFF, prev = OFF;

inline void setColor(uint8_t r,uint8_t g,uint8_t b){
  strip.setPixelColor(0, strip.Color(r,g,b));
  strip.show();
}

void setup() {
  strip.begin();
  strip.clear();
  strip.show();

  pinMode(BTN_RED,  INPUT_PULLUP);
  pinMode(BTN_BLUE, INPUT_PULLUP);

  Serial.begin(115200);
  delay(50);
  Serial.println("\nStart: GPIO4/5 INPUT_PULLUP, GND active");
}

void loop() {
  unsigned long now = millis();

  bool redTrig  = (digitalRead(BTN_RED)  == LOW);
  bool blueTrig = (digitalRead(BTN_BLUE) == LOW);

  // 判定模式（红优先）
  if (redTrig)       mode = RED_ON;
  else if (blueTrig) mode = BLUE_BLINK;
  else               mode = OFF;

  // —— 状态变化：先执行灯的动作（不受打印节流影响）——
  if (mode != prev) {
    switch (mode) {
      case OFF:
        blueOn = false;
        strip.clear(); strip.show();
        break;
      case RED_ON:
        blueOn = false;
        setColor(255,0,0);
        break;
      case BLUE_BLINK:
        blueOn = true;
        lastToggle = now;
        setColor(0,0,255);   // 进入即亮
        break;
    }
    prev = mode;
  }

  // —— 仅打印节流：间隔到才打印一次 —— 
  if (mode != lastPrintedMode && (now - lastPrintTs) >= MIN_PRINT_GAP) {
    switch (mode) {
      case OFF:        Serial.println("Mode: OFF"); break;
      case RED_ON:     Serial.println("Mode: RED"); break;
      case BLUE_BLINK: Serial.println("Mode: BLUE"); break;
    }
    lastPrintedMode = mode;
    lastPrintTs = now;
  }

  // 从触发->松开 时，开启 OFF 窗口强制清灯
  static bool prevRedTrig = false, prevBlueTrig = false;
  if (prevRedTrig && !redTrig)   offHoldUntil = now + OFF_HOLD_MS;
  if (prevBlueTrig && !blueTrig) offHoldUntil = now + OFF_HOLD_MS;
  prevRedTrig  = redTrig;
  prevBlueTrig = blueTrig;

  // 蓝灯非阻塞闪烁
  if (mode == BLUE_BLINK) {
    if (now - lastToggle >= BLINK_INTERVAL) {
      lastToggle = now;
      blueOn = !blueOn;
      if (blueOn) setColor(0,0,255);
      else        { strip.clear(); strip.show(); }
    }
    return;
  }

  // OFF 模式：强制清
  if (now <= offHoldUntil) {
    strip.clear(); strip.show();
  }
}
