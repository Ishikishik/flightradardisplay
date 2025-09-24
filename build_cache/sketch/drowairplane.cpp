#line 1 "/Users/kawashimasatoshishin/kawaradardisplay/kawaeyes/drowairplane.cpp"
#include "drowairplane.hpp"

void drowairplane(float st_lon, float st_lat, float now_lon, float now_lat, const char* code){
    float vecsize = sqrt(pow((now_lon - st_lon),2) + pow((now_lat -  st_lat),2));
    int v_lon = ((now_lon - st_lon)/vecsize * pic_lon) * 0.3;//いい感じに調整
    int v_lat = - ((now_lat -  st_lat)/vecsize * pic_lat) * 0.3;
    const int lonpic_plane = (now_lon - lon_min) * pic_lon;
    const int latpic_plane = (lat_max - now_lat) * pic_lat;
    //lcd.drawLine(lonpic_plane, latpic_plane , lonpic_plane + v_lon, latpic_plane + v_lat,TFT_GREEN ); // ２点間の直線
    lcd.fillTriangle(lonpic_plane, latpic_plane, lonpic_plane - v_lon - v_lat * 0.2, latpic_plane - v_lat + v_lon * 0.2, lonpic_plane - v_lon + v_lat * 0.2, latpic_plane - v_lat - v_lon * 0.2,  TFT_GREEN); // ３点間の三角形の外周
    lcd.drawFastHLine (lonpic_plane - v_lon - 25, latpic_plane - v_lat, 20, TFT_GREEN);  // 水平線
    lcd.drawFastVLine (lonpic_plane - v_lon - 25, latpic_plane - v_lat - 20, 20, TFT_GREEN);  // 水平線

    lcd.setTextSize(1);                        // 文字サイズ 2倍
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);   // 緑文字、黒背景
    lcd.setCursor(lonpic_plane - v_lon - 50, latpic_plane - v_lat - 20);                     // 表示位置
    lcd.print(code);
}