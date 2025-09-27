#ifndef GETJSON_HPP
#define GETJSON_HPP

#include <WiFi.h>
#include <ArduinoJson.h>
#include "lgfxesp32c3.hpp"
#include "library.hpp"

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
extern MyLGFX lcd;                 // 描画先
extern LGFX_Sprite mapSprite;      // 地図
extern LGFX_Sprite planeA;         // ダブルバッファ用スプライト A
extern LGFX_Sprite planeB;         // ダブルバッファ用スプライト B
extern LGFX_Sprite* frontPlane;    // 表バッファ
extern LGFX_Sprite* backPlane;     // 裏バッファ
// 関数プロトタイプ
void setupwifi(const char* ssid, const char* pass);
void showairplane(const char* ip, float lon_min, float pic_lon, float lat_max, float pic_lat);
void showjson(const char* ip);
#endif
