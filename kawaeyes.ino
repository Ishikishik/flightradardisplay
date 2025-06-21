#include <LovyanGFX.hpp>
#include "lgfxesp32c3.hpp"

MyLGFX lcd;  // LGFX → MyLGFX に変更

void setup(void) {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(128);
  lcd.setColorDepth(24);
  lcd.fillScreen(TFT_DARKGRAY);  // OK
  lcd.fillCircle(100, 100, 50, TFT_YELLOW);  // 円の塗りつぶし
}

void loop(void) {
  uint16_t tX, tY;
  if (lcd.getTouch(&tX, &tY)) {
    Serial.printf("Touched: (%d, %d)\n", tX, tY);
    lcd.setColor(TFT_YELLOW);  // ✅ 修正: lgfx::Yellow → TFT_YELLOW
    lcd.drawCircle(tX, tY, 20);
  }
}
