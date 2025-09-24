#line 1 "/Users/kawashimasatoshishin/kawaradardisplay/kawaeyes/drowairplane.hpp"
#ifndef DROW_AIRPLANE_HPP
#define DROW_AIRPLANE_HPP

#include <LovyanGFX.hpp>
#include "lgfxesp32c3.hpp"  // lcdを使うため
#include <cmath>

// ディスプレイ座標変換用の extern 定義
extern const float lon_min;
extern const float lon_max;
extern const float lat_min;
extern const float lat_max;
extern const float pic_lon;
extern const float pic_lat;
extern MyLGFX lcd;

// 関数宣言
void drowairplane(float st_lon, float st_lat, float now_lon, float now_lat, const char* code);

#endif
