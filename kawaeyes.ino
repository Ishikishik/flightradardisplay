#include <LovyanGFX.hpp>
#include "lgfxesp32c3.hpp"
#include "mapdata.hpp"

MyLGFX lcd;  // LGFX → MyLGFX に変更

void setup(void) {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(128);
  lcd.setColorDepth(24);
  
  //背景(日本地図を描写)
  lcd.fillScreen(TFT_DARKGRAY);  // OK
  for (int i = 0; i < NUM_LINES; ++i) {
    lcd.drawLine(lines[i][0], lines[i][1], lines[i][2], lines[i][3], TFT_GREEN);
  }
}

void loop(void) {

}
