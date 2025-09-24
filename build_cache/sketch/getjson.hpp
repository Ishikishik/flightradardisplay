#line 1 "/Users/kawashimasatoshishin/kawaradardisplay/kawaeyes/getjson.hpp"
#ifndef GETJSON_HPP
#define GETJSON_HPP

#include <WiFi.h>
#include <ArduinoJson.h>
#include "lgfxesp32c3.hpp"
#include "drowairplane.hpp"

// 飛行機の状態管理
struct PlaneState {
  char callsign[10];
  float prev_lat;
  float prev_lon;
  bool active;
};

// 外部から使えるグローバル変数
extern PlaneState planes[100];
extern WiFiClient client;
extern MyLGFX lcd;

// 関数プロトタイプ
void setupwifi(const char* ssid, const char* pass);
void getjson(const char* ip);

#endif
