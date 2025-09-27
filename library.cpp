#include "library.hpp"

// 便情報の構造体を定義
struct FlightInfo {
  char callsign[7];  // 便名（最大9文字＋NULL）
  float latitude;     // 緯度
  float longitude;    // 経度
};

// 100機分のデータを格納できる配列
FlightInfo flights[100];

WiFiClient client;       // clientの実体もここに

void setupwifi(const char* ssid, const char* pass) {
  WiFi.begin(ssid, pass);

  int16_t cx = lcd.width()  / 2;
  int16_t cy = lcd.height() / 2;
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);  // 緑文字・黒背景
  lcd.setTextSize(2);                      // お好みでサイズ変更

  while (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(cx - 60, cy - 8);         // 文字幅に合わせてオフセット
    lcd.print("Connecting WiFi...");
    delay(500);
  }
}





/*航空機とコードを地図上に表示するコード*/
void showairplane(const char* ip, float lon_min, float pic_lon, float lat_max, float pic_lat){
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

backPlane->fillSprite(TFT_BLACK);

for (JsonPair kv : doc.as<JsonObject>()) {
  JsonArray data = kv.value().as<JsonArray>();
  if (data[1].as<float>() == 0 || data[2].as<float>() == 0) {
    continue;   
  }
  const char* callsign = data[16];
  float lat = (lat_max - data[1].as<float>()) * pic_lat;
  float lon = (data[2].as<float>() - lon_min) * pic_lon;


  if (lat != 0 && lon != 0 && strlen(callsign) > 0) {
    backPlane->fillCircle(lon, lat , 2, 1); 
  }
}
  } else {
    delay(1000);
    showairplane(ip, lon_min, pic_lon, lat_max, pic_lat); 
    return;
  }
  mapSprite.pushSprite(0, 0);                    
  backPlane->pushSprite(0, 0 , 0);
  std::swap(frontPlane, backPlane);
}





















/*コールサインをそのまま表示するコード*/
void showjson(const char* ip){
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
    delay(1000);
    showjson(ip); // もう一度だけ再試行
    return;
  }
}




