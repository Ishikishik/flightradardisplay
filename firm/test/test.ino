#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

constexpr int TFT_DC = 9;
constexpr int TFT_RST = 8;
constexpr int TFT_MOSI = 7;
constexpr int TFT_SCK = 6;
constexpr int TFT_BL = 5;
constexpr int PHOTO_ADC = 4;
constexpr int PHOTO_PWR = 3;

Adafruit_ST7789 tft(&SPI, -1, TFT_DC, TFT_RST);

struct Button {
  const char* name;
  int pin;
  bool raw = false;
  bool pressed = false;
  uint32_t changed = 0;
  uint32_t count = 0;
};

Button buttons[] = {
  {"LEFT", 10},
  {"SLEEP", 1},
  {"RIGHT", 0}
};

bool inverted = false;
bool backlight = true;
uint32_t lastDraw = 0;
uint32_t lastLog = 0;
float photoBase = 0;
float photoPeak = 0;
int photoADC = 0;
uint32_t photoPulses = 0;
bool photoPulse = false;

void updateButtons() {
  for (auto &b : buttons) {
    bool sample = digitalRead(b.pin) == LOW;
    uint32_t now = millis();

    if (sample != b.raw) {
      b.raw = sample;
      b.changed = now;
    }

    if (sample != b.pressed && now - b.changed >= 25) {
      b.pressed = sample;

      if (b.pressed) {
        b.count++;
        Serial.printf("%s pressed: %lu\n", b.name,
                      (unsigned long)b.count);

        if (b.pin == 1) {
          inverted = !inverted;
          tft.invertDisplay(inverted);
        }
      }
    }
  }
}

void updatePhoto() {
  photoADC = analogRead(PHOTO_ADC);
  if (photoBase == 0) photoBase = photoADC;

  float difference = abs(photoADC - photoBase);
  photoBase += (photoADC - photoBase) * 0.002f;
  photoPeak *= 0.995f;

  if (difference > photoPeak) photoPeak = difference;

  if (difference > 100 && !photoPulse) {
    photoPulses++;
    photoPulse = true;
  }
  if (difference < 40) photoPulse = false;
}

void drawScreen() {
  tft.fillRect(0, 0, 240, 240, ST77XX_BLACK);

  tft.fillRect(0, 0, 240, 26, ST77XX_CYAN);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 5);
  tft.print("KAWAEYES TEST");

  for (int i = 0; i < 3; i++) {
    auto &b = buttons[i];
    int y = 37 + i * 29;
    tft.setTextSize(2);
    tft.setTextColor(b.pressed ? ST77XX_GREEN : ST77XX_WHITE);
    tft.setCursor(8, y);
    tft.print(b.name);
    tft.setCursor(100, y);
    tft.print(b.pressed ? "DOWN" : "UP");
    tft.setTextSize(1);
    tft.setCursor(188, y + 6);
    tft.printf("%lu", (unsigned long)b.count);
  }

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 128);
  tft.print("LED1: DISABLED (GPIO9 CONFLICT)");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 146);
  tft.printf("BACKLIGHT: %s   INV: %s",
             backlight ? "ON" : "OFF",
             inverted ? "ON" : "OFF");

  tft.drawFastHLine(8, 164, 224, ST77XX_CYAN);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 173);
  tft.print("PHOTO SENSOR (AC SIGNAL)");

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 188);
  tft.printf("ADC:%4d", photoADC);
  tft.setCursor(132, 188);
  tft.printf("P:%3d", (int)photoPeak);

  tft.setTextSize(1);
  tft.setCursor(8, 211);
  tft.printf("Pulse count: %lu",
             (unsigned long)photoPulses);

  tft.drawRect(8, 226, 224, 10, ST77XX_WHITE);
  int level = constrain((int)(photoPeak / 5), 0, 220);
  tft.fillRect(10, 228, level, 6, ST77XX_GREEN);
}

void setup() {
  Serial.begin(115200);

  for (auto &b : buttons) {
    pinMode(b.pin, INPUT_PULLUP);
  }

  pinMode(PHOTO_PWR, OUTPUT);
  digitalWrite(PHOTO_PWR, HIGH);
  pinMode(PHOTO_ADC, INPUT_PULLDOWN);
  analogReadResolution(12);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, -1);
  tft.init(240, 240);
  tft.setRotation(0);

  tft.fillScreen(ST77XX_RED);
  delay(300);
  tft.fillScreen(ST77XX_GREEN);
  delay(300);
  tft.fillScreen(ST77XX_BLUE);
  delay(300);
  tft.fillScreen(ST77XX_BLACK);

  Serial.println("Kawaeyes hardware test started");
  Serial.println("WARNING: Do not press LED1.");
}

void loop() {
  updateButtons();
  updatePhoto();

  if (millis() - lastDraw >= 150) {
    lastDraw = millis();
    drawScreen();
  }

  if (millis() - lastLog >= 1000) {
    lastLog = millis();
    Serial.printf("Photo ADC=%d Peak=%.1f Pulses=%lu\n",
                  photoADC, photoPeak,
                  (unsigned long)photoPulses);
  }
  delay(1);
}
