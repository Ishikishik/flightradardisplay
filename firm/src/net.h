

#define NET_ADD_MAX 8   // 足せる数

struct NetAp { char ssid[33], pass[65]; };
NetAp   netAdded[NET_ADD_MAX];
uint8_t netAddedN = 0;
bool    netWifi = false;     // Wi-Fi につながっているか
bool    netServer = false;   // サーバー(WebSocket)につながっているか
const char* netSsid = "";    // つなごうとしている(つながった) SSID
Preferences netPrefs;

WebSocketsClient netWs;

// 接続確認の画面
void netShow() {
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font0);   // show.h でフォントを変えているので標準に戻す
  lcd.setTextSize(2);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(netWifi ? TFT_GREEN : TFT_YELLOW);
  lcd.drawString(netWifi ? "wifi: success" : "wifi: connecting...", LCD_W / 2, LCD_H / 2 - 12);
  lcd.setTextColor(netServer ? TFT_GREEN : TFT_YELLOW);
  lcd.drawString(netServer ? "server: success" : "server: connecting...", LCD_W / 2, LCD_H / 2 + 12);
  lcd.setTextColor(TFT_DARKGREY);
  lcd.drawString(netSsid, LCD_W / 2, LCD_H / 2 + 48);
  lcd.setTextDatum(textdatum_t::top_left);   // 他の描画に影響しないよう戻す
}

//接続数を足すやつ
// 一覧の先頭(一番優先)に足して保存する。同じ SSID があれば先頭へ移してパスワードを上書き。満杯なら一番古いものを消す
bool netAdd(const char* ssid, const char* pass) {
  int i = 0;
  while (i < netAddedN && strcmp(netAdded[i].ssid, ssid)) ++i;
  if (i == netAddedN && netAddedN < NET_ADD_MAX) ++netAddedN;   // 新しい SSID
  if (i == NET_ADD_MAX) i = NET_ADD_MAX - 1;                     // 満杯: 一番古い(最後)を押し出す
  memmove(&netAdded[1], &netAdded[0], sizeof(NetAp) * i);       // 前にあったものを 1 つずつ後ろへ
  strlcpy(netAdded[0].ssid, ssid, sizeof(netAdded[0].ssid));
  strlcpy(netAdded[0].pass, pass, sizeof(netAdded[0].pass));
  netPrefs.putBytes("list", netAdded, sizeof(NetAp) * netAddedN);
  return true;
}

// スキャン結果(n 件)に ssid があれば接続を試す。つながれば true
bool netTry(int n, const char* ssid, const char* pass) {
  for (int j = 0; j < n; ++j) {
    if (WiFi.SSID(j) != ssid) continue;
    netSsid = ssid;
    netShow();
    WiFi.begin(ssid, pass);
    return WiFi.waitForConnectResult(15000) == WL_CONNECTED;
  }
  return false;
}

void netConnect() {
  netWifi = false;
  netShow();
  int n = WiFi.scanNetworks();
  bool ok = false;
  for (int i = 0; i < netAddedN && !ok; ++i)   ok = netTry(n, netAdded[i].ssid, netAdded[i].pass);   // 新しく足したものから
  for (auto& w : WIFI_LIST)                    if (!ok) ok = netTry(n, w[0], w[1]);                        // 最後に secret.h のもの
  netWifi = ok;
  netShow();
}

void netEvent(WStype_t type, uint8_t* data, size_t len) {
  if (type == WStype_CONNECTED)    { netServer = true;  netShow(); }
  if (type == WStype_DISCONNECTED) { if (netServer) { netServer = false; netShow(); } }
  if (type == WStype_BIN)          flightsDecode(data, len);   // flights.h で解読
}

void netInit() {
  netWs.beginSSL(WS_HOST, WS_PORT, WS_PATH, "", "");   // 証明書は検証しない(TODO)
  netWs.onEvent(netEvent);
  netWs.setReconnectInterval(5000);       // 切れたら 5 秒ごとに再接続
  netWs.enableHeartbeat(15000, 3000, 2);  // 応答がなければ切断して再接続
  netPrefs.begin("wifi");
  netAddedN = netPrefs.getBytes("list", netAdded, sizeof(netAdded)) / sizeof(NetAp);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);   // つなぎ直しは netLoop で優先順位どおりに行う
}

// ---------------------------------------------------------------- 光で Wi-Fi を受け取る(otter-device-link)
// 左右どちらかのボタンを長押しすると受け取る画面になる。スマホでページを開き、画面の点滅を光センサーで読む
// プロトコルは otter-device-link の README を参照。受け取れたら netAdd() で保存して、つなぎ直す
#define LINK_URL     "https://ishikishik.github.io/otter-device-link/"
#define LINK_LONG    1500     // 長押しと見なす時間 [ms]
#define LINK_TIMEOUT 180000   // 何も受け取らずに待つ最大時間 [ms]
#define LINK_BLOCK   10       // 光センサーの値をこの時間 [ms] ごとに平均して判定する

struct NetLink {
  int lo = 4095, hi = 0;                  // 最近の一番暗い値と明るい値(しきい値はこの中間)
  bool light = false;                     // 今の明暗
  uint32_t edge = 0, decay = 0, green = 0, T = 0;
  long acc = 0; int cnt = 0; uint32_t blk = 0;   // LINK_BLOCK ごとの平均を取るための合計・個数・区切りの時刻
  bool pend = false; uint32_t pendT = 0;          // 明暗が切り替わりかけている(次の区間も同じなら本物)
  uint32_t lastG = 0, lastB = 0;                  // 最後に判定した緑と黒の長さ(画面に出して調整に使う)
  int mode = 0;                           // 0 開始待ち / 1 開始の後半待ち / 2 SSID / 3 パスワード / 4 チェックサム / 5 終わり
  String ssid, pass;
  int sum = -1, bits = 0, nbits = 0;
  bool ok = false;
};
NetLink lk;

void netLinkBit(int bit) {
  lk.bits = lk.bits << 1 | bit;
  if (++lk.nbits < 8) return;
  if (lk.mode == 2 && lk.bits < 0x80) lk.ssid += char(lk.bits);
  if (lk.mode == 3 && lk.bits < 0x80) lk.pass += char(lk.bits);
  if (lk.mode == 4) lk.sum = lk.bits;
  lk.bits = lk.nbits = 0;
}

// 緑 g [ms] と黒 b [ms] の 1 組を判定する
void netLinkPulse(uint32_t g, uint32_t b) {
  lk.lastG = g; lk.lastB = b;
  if (lk.mode == 0 || lk.mode == 5) {   // 開始の前半(緑 8T・黒 4T)。ここで T を測る
    if (g >= 160 && g * 10 > b * 16 && g * 10 < b * 24) { lk.T = (g + b) / 12; lk.mode = 1; }
    return;
  }
  int G = (g + lk.T / 2) / lk.T, B = (b + lk.T / 2) / lk.T;   // T の何倍か
  auto near = [](int v, int t) { return abs(v - t) <= 1; };
  if (near(G, 4) && near(B, 1)) {       // 区切り(開始の後半 → SSID → パスワード → チェックサム → 終わり)
    if (lk.mode == 1) { lk.ssid = lk.pass = ""; lk.sum = -1; }
    lk.bits = lk.nbits = 0;
    if (++lk.mode == 5) {
      String t = lk.ssid + lk.pass;
      uint8_t sum = 0;
      for (unsigned i = 0; i < t.length(); ++i) sum += t[i];   // SSID とパスワードの全バイトの合計の下位 8 ビット
      lk.ok = lk.sum == sum;
    }
  } else if (lk.mode == 1) lk.mode = 0;   // 開始の合図ではなかった
  else if (near(G, 1) && near(B, 1)) netLinkBit(0);
  else if (near(G, 1) && near(B, 3)) netLinkBit(1);
}

void netLinkRead() {
  // LINK_BLOCK ごとの平均で判定する。スマホの画面の PWM 調光(数百 Hz)や、照明のちらつき(100/120Hz)をならす
  lk.acc += analogRead(ALS); ++lk.cnt;
  uint32_t now = millis();
  if (now - lk.blk < LINK_BLOCK) return;
  int v = lk.acc / lk.cnt;
  lk.acc = lk.cnt = 0; lk.blk = now;

  lk.lo = min(lk.lo, v);
  lk.hi = max(lk.hi, v);
  if (now - lk.decay >= 10) { lk.decay = now; if (lk.lo < lk.hi) { lk.lo++; lk.hi--; } }   // 明るさの変化に追従
  if (lk.hi - lk.lo < 30) return;                                    // まだ点滅していない
  int mid = (lk.lo + lk.hi) / 2, m = (lk.hi - lk.lo) / 8;
  bool l = lk.light ? v > mid - m : v > mid + m;                     // 境目でばたつかないよう余裕を持たせる
  // 最後の「終わり」(緑 4T・黒 1T)のあとは黒のまま次の緑が来ないので、黒が 5T を超えたらそこで判定する
  if (!l && !lk.light && lk.green && lk.T && now - lk.edge > 5 * lk.T) { netLinkPulse(lk.green, lk.T); lk.green = 0; }
  if (l == lk.light) { lk.pend = false; return; }
  if (!lk.pend) { lk.pend = true; lk.pendT = lk.blk - LINK_BLOCK; return; }   // 1 区間だけの変化は様子を見る
  uint32_t d = lk.pendT - lk.edge;                                   // 2 区間続いたので本物: 直前の長さ
  if (lk.light) lk.green = d; else netLinkPulse(lk.green, d);
  lk.light = l; lk.edge = lk.pendT; lk.pend = false;
}

void netLinkDraw() {
  const char* st[] = {"waiting", "start", "SSID", "password", "checksum", ""};
  lcd.fillRect(0, 140, LCD_W, LCD_H - 140, TFT_BLACK);
  lcd.setTextColor(TFT_GREEN);
  lcd.setCursor(8, 148); lcd.printf("SSID: %s", lk.ssid.c_str());
  lcd.setCursor(8, 168); lcd.printf("PASS: %d chars", lk.pass.length());   // パスワードは画面に出さない
  lcd.setCursor(8, 196);
  if (lk.mode == 5) { lcd.setTextColor(lk.ok ? TFT_GREEN : TFT_RED); lcd.print(lk.ok ? "OK! saved" : "NG - send again"); }
  else              lcd.printf("%s  T=%lums", st[lk.mode], (unsigned long)lk.T);
  lcd.setTextColor(TFT_DARKGREY);
  lcd.setCursor(8, 220); lcd.printf("last G=%lu B=%lu ms", (unsigned long)lk.lastG, (unsigned long)lk.lastB);   // 調整用
}

void netLink() {
#ifdef ALS
  pinMode(ALS_PWR, OUTPUT);
  digitalWrite(ALS_PWR, HIGH);   // 光センサーの電源を入れる
  lk = NetLink();
  lk.edge = millis();

  lcd.fillScreen(TFT_BLACK);
  lcd.qrcode(LINK_URL, 4, 4, 128, 3);
  lcd.setFont(&fonts::AsciiFont8x16);
  lcd.setTextSize(1);
  lcd.setTextColor(TFT_WHITE);
  const char* help[] = {"Wi-Fi setup", "1. Scan QR", "2. Brightness max", "3. Sensor on the", "   upper half", "4. Press START", "any button: back"};
  for (int i = 0; i < 7; ++i) { lcd.setCursor(140, 4 + i * 18); lcd.print(help[i]); }
  netLinkDraw();

  auto pressed = [] { return digitalRead(SW_L) == LOW || digitalRead(SW_R) == LOW || digitalRead(SW_P) == LOW; };
  while (pressed()) delay(10);   // 長押ししたボタンを離すまで待つ
  uint32_t start = millis();
  int shown = 0;
  while (millis() - start < LINK_TIMEOUT && !pressed()) {   // どれかのボタンで戻る
    netLinkRead();
    int now = lk.mode * 10000 + lk.ssid.length() * 100 + lk.pass.length() + lk.lastG + lk.lastB * 7;
    if (now != shown) { shown = now; netLinkDraw(); start = millis(); }   // 変化があったら描き直し、待ち時間も延ばす
    if (lk.mode == 5 && lk.ok) {
      netAdd(lk.ssid.c_str(), lk.pass.c_str());
      delay(2000);
      WiFi.disconnect();   // つなぎ直して、新しい Wi-Fi も候補に入れる
      break;
    }
  }
  while (pressed()) delay(10);
  digitalWrite(ALS_PWR, LOW);
  netShow();
#endif
}

// 左右どちらかのボタンを長押ししたら netLink()
void netLinkCheck() {
#ifdef SW_L
  static uint32_t down = 0;
  if (digitalRead(SW_L) != LOW && digitalRead(SW_R) != LOW) { down = 0; return; }
  if (!down) down = millis();
  if (millis() - down >= LINK_LONG) { down = 0; netLink(); }
#endif
}

void netLoop() {
  netLinkCheck();
  netWifi = WiFi.status() == WL_CONNECTED;
  if (!netWifi) { netServer = false; netConnect(); }
  else netWs.loop();
}
