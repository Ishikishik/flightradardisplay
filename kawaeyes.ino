#include "drowairplane.hpp"
#include "mapdata.hpp"

MyLGFX lcd;  // LGFX → MyLGFX に変更


const float lon_min = 135.755; //画面西(左)端の経度
const float lon_max = 142.954; //画面東(右)端の経度
const float lat_min = 33.451; //画面下(南)端の経度
const float lat_max = 39.865; //画面上(北)端の経度
const float lon_home = 139.529; //アンテナの経度
const float lat_home = 35.651; //アンテナの緯度

//1度が何ピクセルに当たるかを計算(ディスプレイの大きさに合わせて調整)
const float pic_lon = 320 / (lon_max - lon_min); 
const float pic_lat = 240 / (lat_max - lat_min);
  

// 便情報の構造体を定義
struct FlightInfo {
  char callsign[7];  // 便名（最大9文字＋NULL）
  float latitude;     // 緯度
  float longitude;    // 経度
};

// 100機分のデータを格納できる配列
FlightInfo flights[100];

void setup(void) {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(128);
  lcd.setColorDepth(24);

  //自宅の位置をピクセル座標化
  const int lonpic_home = (lon_home - lon_min) * pic_lon;
  const int latpic_home = (lat_max - lat_home) * pic_lat;
  //背景(日本地図を描写)
  lcd.fillScreen(TFT_DARKGRAY);  // OK
  for (int i = 0; i < NUM_LINES; ++i) {
    lcd.drawLine(lines[i][0], lines[i][1], lines[i][2], lines[i][3], TFT_GREEN);
  }
  //自宅に丸い点を打つ
  lcd.fillCircle(lonpic_home, latpic_home , 2, TFT_GREEN);  
  //50kmと100kmに円を描く(要修正)
  //lcd.drawCircle(lonpic_home, latpic_home , pic_long, TFT_GREEN);  //(要修正)
  //lcd.drawCircle(lonpic_home, latpic_home , pic_long * 2, TFT_GREEN);  
}


void loop(void) {

drowairplane(136.882, 35.169, 137.726, 34.711, "JJ5STB");

}
