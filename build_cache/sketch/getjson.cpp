#line 1 "/Users/kawashimasatoshishin/kawaradardisplay/kawaeyes/getjson.cpp"
#include "getjson.hpp"

PlaneState planes[100];  // 実体の定義
WiFiClient client;       // clientの実体もここに

void setupwifi(const char* ssid, const char* pass) {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.drawCircle(200, 100, 50, TFT_GREEN);  
    delay(500);
  }
  lcd.drawCircle(10, 10, 20, TFT_GREEN);  
}



void getjson(const char* ip){
    // 正しい形式でIPとポートを分けて接続
  if (client.connect(ip, 8754)) {
    client.println("GET /flights.json HTTP/1.0");
    client.print("Host: ");
    client.println(ip);
    client.println("Connection: close");
    client.println();
    // HTTPヘッダをスキップ
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }

    // 本文をすべて読み込む
    String payload = "";
    while (client.available()) {
      payload += client.readString();
    }

    // JSONバッファを確保（容量は必要に応じて調整）
    const size_t capacity = 64 * JSON_OBJECT_SIZE(100) + 10000;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      return;
    }

    lcd.fillScreen(TFT_BLACK);

   int y = 10; // 最初の表示位置Y座標
lcd.fillScreen(TFT_BLACK);  // 画面をクリア

for (JsonPair kv : doc.as<JsonObject>()) {
  JsonArray data = kv.value().as<JsonArray>();

  const char* callsign = data[16];
  float lat = data[1];
  float lon = data[2];

  if (lat != 0 && lon != 0 && strlen(callsign) > 0) {
    // 1行に1件表示（例：JJ5STB: 35.12, 139.44）
    lcd.setCursor(10, y);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.printf("%s: %.2f, %.2f", callsign, lat, lon);

    y += 12;  // 次の行へ（文字サイズ1なら12ピクセルくらい空ける）
    
    // 画面の高さを超えたら、それ以上は表示しない（表示崩れ対策）
    if (y > lcd.height() - 12) break;
  }
}
  } else {
    Serial.println("接続に失敗しました");
  }

  delay(10000); // 次の更新まで待機
}



/*コールサインをそのまま表示するコード*/
void showjson2(const char* ip){
  // 正しい形式でIPとポートを分けて接続
  if (client.connect(ip, 8754)) {
    client.println("GET /flights.json HTTP/1.0");
    client.print("Host: ");
    client.println(ip);
    client.println("Connection: close");
    client.println();
    // HTTPヘッダをスキップ
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }

    // 本文をすべて読み込む
    String payload = "";
    while (client.available()) {
      payload += client.readString();
    }

    // JSONバッファを確保（容量は必要に応じて調整）
    const size_t capacity = 64 * JSON_OBJECT_SIZE(100) + 10000;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("JSON Error: ");
      Serial.println(error.f_str());
      return;
    }

    lcd.fillScreen(TFT_BLACK);

   int y = 10; // 最初の表示位置Y座標
lcd.fillScreen(TFT_BLACK);  // 画面をクリア

for (JsonPair kv : doc.as<JsonObject>()) {
  JsonArray data = kv.value().as<JsonArray>();

  const char* callsign = data[16];
  float lat = data[1];
  float lon = data[2];

  if (lat != 0 && lon != 0 && strlen(callsign) > 0) {
    // 1行に1件表示（例：JJ5STB: 35.12, 139.44）
    lcd.setCursor(10, y);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.printf("%s: %.2f, %.2f", callsign, lat, lon);

    y += 12;  // 次の行へ（文字サイズ1なら12ピクセルくらい空ける）
    
    // 画面の高さを超えたら、それ以上は表示しない（表示崩れ対策）
    if (y > lcd.height() - 12) break;
  }
}
  } else {
    Serial.println("接続に失敗しました");
  }

  delay(10000); // 次の更新まで待機

}




/*

#include <WiFi.h>
#include <ArduinoJson.h>
#include "lgfxesp32c3.hpp"

MyLGFX lcd;  // LovyanGFX のインスタンス

WiFiClient client;

void setup() {
  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(128);
  lcd.setColorDepth(24);

  WiFi.begin("Buffalo-3F70", "r4ey8h3jnfwgp");

  // Wi-Fi接続待ち
  while (WiFi.status() != WL_CONNECTED) {
    lcd.drawCircle(200, 100, 50, TFT_GREEN);  
    delay(500);
  }

  // 接続できた場合
  lcd.drawCircle(10, 10, 20, TFT_GREEN);  
}

void loop() {   
  // 正しい形式でIPとポートを分けて接続
  if (client.connect("192.168.11.58", 8754)) {
    client.println("GET /flights.json HTTP/1.0");
    client.println("Host: 192.168.11.58");
    client.println("Connection: close");
    client.println();
    // HTTPヘッダをスキップ
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }

    // 本文をすべて読み込む
    String payload = "";
    while (client.available()) {
      payload += client.readString();
    }

    // JSONバッファを確保（容量は必要に応じて調整）
    const size_t capacity = 64 * JSON_OBJECT_SIZE(100) + 10000;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("JSON Error: ");
      Serial.println(error.f_str());
      return;
    }

    lcd.fillScreen(TFT_BLACK);

   int y = 10; // 最初の表示位置Y座標
lcd.fillScreen(TFT_BLACK);  // 画面をクリア

for (JsonPair kv : doc.as<JsonObject>()) {
  JsonArray data = kv.value().as<JsonArray>();

  const char* callsign = data[16];
  float lat = data[1];
  float lon = data[2];

  if (lat != 0 && lon != 0 && strlen(callsign) > 0) {
    // 1行に1件表示（例：JJ5STB: 35.12, 139.44）
    lcd.setCursor(10, y);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.printf("%s: %.2f, %.2f", callsign, lat, lon);

    y += 12;  // 次の行へ（文字サイズ1なら12ピクセルくらい空ける）
    
    // 画面の高さを超えたら、それ以上は表示しない（表示崩れ対策）
    if (y > lcd.height() - 12) break;
  }
}
  } else {
    Serial.println("接続に失敗しました");
  }

  delay(10000); // 次の更新まで待機
}
  */