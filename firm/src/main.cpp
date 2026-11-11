// ---- ライブラリ
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <LovyanGFX.hpp>
#include <LittleFS.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "config.h"
#include "secret.h"
#include "lcd.h"
MyLGFX lcd;
#include "table.h"     // 地図・国・航空会社の表(maketable/maketable.py で生成)
#include "flights.h"   // 作る: バイナリの解読と機体の一覧
#include "net.h"       // 作る: Wi-Fi と WebSocket(受け取ったら flights.h へ)
#include "show.h"      // 見せる


void setup() {
  lcd.init();
  lcd.setRotation(LCD_ROT);
  lcd.setBrightness(128);
  showInit();
  flightsLoad();   // 前回スリープしたときの一覧(軌跡)を引き継ぐ
  netInit();
}

bool ready = false;   // Wi-Fi とサーバーの両方につながっているか

void loop() {
  netLoop();
  bool now = netWifi && netServer;
  if (now && !ready) delay(1000);   // success の表示を少し見せる
  ready = now;
  if (ready) showLoop();            // 受け取ったバイナリを文字列で表示
  delay(10);
}
