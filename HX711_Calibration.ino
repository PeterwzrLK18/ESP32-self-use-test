#include "HX711.h"

#define DT2 20
#define SCK2 21

HX711 scale2;

long emptyRaw = 0;
long loadedRaw = 0;

void setup() {
  Serial.begin(115200);
  scale2.begin(DT2, SCK2);
  Serial.println("请输入命令：noload / load / calc");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "noload") {
      Serial.println("📊 开始采样空载 raw...");
      emptyRaw = scale2.read_average(20);  // 读取20次求平均
      Serial.print("空载 raw 平均值 = ");
      Serial.println(emptyRaw);

    } else if (cmd == "load") {
      Serial.println("📊 开始采样加载后的 raw...");
      loadedRaw = scale2.read_average(20);
      Serial.print("加载 raw 平均值 = ");
      Serial.println(loadedRaw);

    } else if (cmd == "calc") {
      if (emptyRaw == 0 || loadedRaw == 0) {
        Serial.println("⚠️ 请先输入 noload 和 load 进行采样！");
        return;
      }

      Serial.println("请输入砝码重量（单位kg），例如 1.0：");
      while (!Serial.available());
      String weightStr = Serial.readStringUntil('\n');
      weightStr.trim();
      float knownWeight = weightStr.toFloat();

      float scaleFactor = (loadedRaw - emptyRaw) / knownWeight;

      Serial.println("\n✅ 校准完成！");
      Serial.print("建议 scale2.set_scale(");
      Serial.print(scaleFactor, 1);
      Serial.println(");");

      Serial.print("建议 scale2.set_offset(");
      Serial.print(emptyRaw);
      Serial.println(");");
    } else {
      Serial.println("⚠️ 未知命令。请输入 noload / load / calc");
    }
  }
}
