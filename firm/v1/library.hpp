#ifndef GETJSON_HPP
#define GETJSON_HPP


#include "lgfxesp32c3.hpp"

// 飛行機の状態管理
struct PlaneState {
  char callsign[10];
  float prev_lat;
  float prev_lon;
  bool active;
};

// 外部から使えるグローバル変数
extern WiFiClient client;
extern MyLGFX lcd;
extern LGFX_Sprite mapSprite;
extern LGFX_Sprite planeSprite;
extern LGFX_Sprite screenSprite;

// 関数プロトタイプ
void setupwifi(const char* ssid, const char* pass);
void showairplane(const char* ip, float lon_min, float pic_lon, float lat_max, float pic_lat);
void showjson(const char* ip);
#endif
