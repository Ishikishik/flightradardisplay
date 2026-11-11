//カクツキ防止のスプライト(何枚か)
LGFX_Sprite Sprite1(&lcd);
LGFX_Sprite Sprite2(&lcd);
LGFX_Sprite Sprite3(&lcd);
LGFX_Sprite Sprite4(&lcd);
#define C_BLACK 0   // パレット番号
#define C_GREEN 1
#define C_PINK  2
#define C_WHITE 3

LGFX_Sprite showSpr(&lcd);

#define ICON_W 24                              // 国旗・ロゴを描く幅(高さは文字と同じ 16)
#define BOX_W (4 + ICON_W + 4 + 16 * 8 + 4)    // 余白 + 国旗/ロゴ + 間 + 16 文字 + 余白
#define BOX_H (4 * 16 + 8)                     // 4 行 + 余白
LGFX_Sprite infoSpr(&lcd);   // 情報欄。国旗やロゴを本来の色で描くため 16 bit(2 bit の showSpr だと 4 色になってしまう)

#if REV == 1
  #define SHOW_BTN SW_A   // 地図と一覧を切り替えるボタン
#else
  #define SHOW_BTN SW_P   // 押し込み
#endif
bool     showMap  = true;   // true = 地図 / false = 一覧
int      showPage = 0;      // 一覧のページ
uint32_t showSel  = 0;      // 地図で選んでいる機体の ICAO(0 = 選んでいない)

void showInit() {
  showSpr.setColorDepth(2);
  showSpr.createSprite(LCD_W, LCD_H);  
  showSpr.createPalette();
  showSpr.setPaletteColor(C_BLACK, 0x000000U);
  showSpr.setPaletteColor(C_GREEN, 0x00FF00U);
  showSpr.setPaletteColor(C_PINK,  0xFF6CFFU);
  showSpr.setPaletteColor(C_WHITE, 0xFFFFFFU);
  showSpr.setFont(&fonts::AsciiFont8x16);
  infoSpr.setColorDepth(16);
  infoSpr.createSprite(BOX_W, BOX_H);   // 164 x 72 x 2 バイト = 約 23KB
  infoSpr.setFont(&fonts::AsciiFont8x16);
  pinMode(SHOW_BTN, INPUT_PULLUP);   // 押すと LOW
#ifdef SW_L
  pinMode(SW_L, INPUT_PULLUP);
  pinMode(SW_R, INPUT_PULLUP);
#endif
}

// ---- 地図
#define SHOW_INTERVAL 250   // 描き直す間隔 [ms]。1 秒の移動を 4 回に分けて描く
#define PX_LON (LCD_W / (LON_MAX - LON_MIN))   // 経度 1 度あたりのピクセル
#define PX_LAT (LCD_H / (LAT_MAX - LAT_MIN))   // 緯度 1 度あたりのピクセル

int showX(float lon) { return (lon - LON_MIN) * PX_LON; }
int showY(float lat) { return (LAT_MAX - lat) * PX_LAT; }

// 海岸線とアンテナの位置を描く
void drawMap() {
  showSpr.fillScreen(C_BLACK);
  for (auto& l : mapLines) showSpr.drawLine(l[0], l[1], l[2], l[3], C_GREEN);
  showSpr.fillCircle(showX(LON_HOME), showY(LAT_HOME), 2, C_PINK);
}

// (x, y) に、向き deg(北 0、時計回り)の三角を描く
void drawPlane(float x, float y, float deg, int color) {
  // 地図は経度と緯度で 1 ピクセルの距離が違うので、画面上の向きに直す
  float r  = deg * DEG_TO_RAD;
  float dx = sinf(r) * PX_LON / cosf(((LAT_MIN + LAT_MAX) / 2) * DEG_TO_RAD);   // 東西は 1 度が cos(緯度) 倍短い
  float dy = -cosf(r) * PX_LAT;
  float l  = sqrtf(dx * dx + dy * dy);
  dx /= l; dy /= l;                       // 進む向き(長さ 1)
  const float L = 6, W = 3;               // 先端までの長さ、後ろの幅
  showSpr.fillTriangle(x + dx * L,          y + dy * L,
                       x - dx * L - dy * W, y - dy * L + dx * W,
                       x - dx * L + dy * W, y - dy * L - dx * W, color);
}

// 選んだ機体の情報を、4 隅のうち機体が一番少ない場所に出す

// 16 色の画像(table.h の形)を、(x, y) に dw x dh に拡大縮小して描く
void drawIcon(int x, int y, int dw, int dh, const uint8_t* pix, const uint16_t* pal, int sw, int sh) {
  for (int j = 0; j < dh; ++j)
    for (int i = 0; i < dw; ++i) {
      int sx = i * sw / dw, sy = j * sh / dh;
      uint8_t b = pix[(sy * sw + sx) / 2];
      infoSpr.drawPixel(x + i, y + j, pal[sx & 1 ? b & 0x0F : b >> 4]);   // 偶数の x は上位 4 ビット
    }
}

// 選んだ機体の情報を infoSpr に描き、置く場所(4 隅のうち機体が一番少ない所)を bx, by に返す
void drawInfo(const Flight& f, const int* xs, const int* ys, int n, int& bx, int& by) {
  static int corner = 0;      // 前回の場所(同数なら動かさない)
  int cnt[4] = {};
  for (int i = 0; i < n; ++i)
    for (int c = 0; c < 4; ++c) {
      int x = c & 1 ? LCD_W - BOX_W : 0, y = c & 2 ? LCD_H - BOX_H : 0;
      if (xs[i] >= x && xs[i] < x + BOX_W && ys[i] >= y && ys[i] < y + BOX_H) ++cnt[c];
    }
  for (int c = 0; c < 4; ++c) if (cnt[c] < cnt[corner]) corner = c;
  bx = corner & 1 ? LCD_W - BOX_W : 0;
  by = corner & 2 ? LCD_H - BOX_H : 0;

  const int tx = 4 + ICON_W + 4;   // 国旗・ロゴの右の文字の位置
  infoSpr.fillScreen(TFT_BLACK);
  infoSpr.drawRect(0, 0, BOX_W, BOX_H, lcd.color565(0xFF, 0x6C, 0xFF));
  infoSpr.setTextColor(TFT_WHITE);
  infoSpr.setCursor(4, 4);
  if (f.callsign[0]) infoSpr.print(f.callsign); else infoSpr.printf("%06lX", (unsigned long)f.icao);
  infoSpr.setCursor(4, 20);
  if (f.flags & F_GND) infoSpr.print("ground"); else if (f.flags & F_ALT) infoSpr.printf("%ld ft", (long)f.alt); else infoSpr.print("-");

  if (f.flag >= 0) drawIcon(4, 36, FLAG_W, FLAG_H, FLAG_PIX[f.flag], FLAG_PAL[f.flag], FLAG_W, FLAG_H);
  infoSpr.setCursor(tx, 36);
  infoSpr.printf("%.16s", f.country ? f.country : "-");

  if (f.logo >= 0) drawIcon(4 + (ICON_W - LOGO_W) / 2, 52, LOGO_W, LOGO_H, LOGO_PIX[f.logo], LOGO_PAL[f.logo], LOGO_W, LOGO_H);
  infoSpr.setCursor(tx, 52);
  infoSpr.printf("%.16s", f.airline ? f.airline : "-");
}

// showSpr を液晶に送る。情報欄(bx, by)がある場合はその部分だけ送らない
// (地図を送ってから情報欄を重ねると、その間だけ地図が見えて情報欄がちらつくため)
void showPush(int bx, int by) {
  if (bx < 0) { showSpr.pushSprite(0, 0); return; }
  int bot = by + BOX_H;
  if (by > 0)      { lcd.setClipRect(0, 0, LCD_W, by);             showSpr.pushSprite(0, 0); }   // 情報欄より上
  if (bot < LCD_H) { lcd.setClipRect(0, bot, LCD_W, LCD_H - bot);  showSpr.pushSprite(0, 0); }   // 情報欄より下
  lcd.setClipRect(bx ? 0 : BOX_W, by, LCD_W - BOX_W, BOX_H);       showSpr.pushSprite(0, 0);     // 情報欄の横
  lcd.clearClipRect();
}

void showplaneonmap() {
  drawMap();
  uint32_t now = millis();
  int xs[FLIGHT_MAX], ys[FLIGHT_MAX], n = 0;   // 描いた機体の画面上の位置(情報欄の場所決め用)
  const Flight* sel = nullptr;
  for (auto& f : flights) {
    if (!f.icao || !(f.flags & F_POS)) continue;
    // 1 つ前の位置から今の位置へ、前回動いてからの時間に合わせて少しずつ進める
    float t = min(1.0f, (now - f.posTime) / (float)f.posDt);
    Pos p = {f.prev.lat + (f.pos.lat - f.prev.lat) * t, f.prev.lon + (f.pos.lon - f.prev.lon) * t};
    float x = showX(p.lon), y = showY(p.lat);
    if (x < -10 || x > LCD_W + 10 || y < -10 || y > LCD_H + 10) continue;  
    xs[n] = x; ys[n++] = y;
    if (f.icao == showSel) {
      sel = &f;
      // 選んだ機体の軌跡: 残した点を古い順に結び、最後の点から今描いている位置へ
      for (int i = 0; i < f.trailN; ++i) {
        Pos a = flightsTrail(f, i), b = i == f.trailN - 1 ? p : flightsTrail(f, i + 1);
        showSpr.drawLine(showX(a.lon), showY(a.lat), showX(b.lon), showY(b.lat), C_PINK);
      }
    }
    drawPlane(x, y, f.track, f.icao == showSel ? C_PINK : C_WHITE);   
  }
  int bx = -1, by = 0;
  if (sel) drawInfo(*sel, xs, ys, n, bx, by);
  else     showSel = 0;   // 選んでいた機体が消えたら選択を外す
  showPush(bx, by);
  if (bx >= 0) infoSpr.pushSprite(bx, by);
}

// 地図で選ぶ機体を d(+1 / -1)だけ動かす。端を越えたら「選ばない」に戻る
void showSelect(int d) {
  int idx[FLIGHT_MAX], n = 0, cur = -1;
  for (int i = 0; i < FLIGHT_MAX; ++i) {
    if (!flights[i].icao || !(flights[i].flags & F_POS)) continue;
    if (flights[i].icao == showSel) cur = n;
    idx[n++] = i;
  }
  int next = cur < 0 ? (d > 0 ? 0 : n - 1) : cur + d;
  showSel = (next >= 0 && next < n) ? flights[idx[next]].icao : 0;
}

//単純にリストとして表示(flights.h が解読した結果の確認用)
void showList() {
  showSpr.fillScreen(C_BLACK);
  showSpr.setCursor(0, 0);
  showSpr.setTextColor(C_WHITE);   
  showSpr.printf("%-8s %5s %3s %-10s %-10s\n", "CALLSIGN", "ALT", "TRK", "COUNTRY", "AIRLINE");
  showSpr.setTextColor(C_GREEN);
  const int rows = LCD_H / 16 - 2;  
  int count = 0;
  for (auto& f : flights) if (f.icao) ++count;
  int pages = max(1, (count + rows - 1) / rows);
  showPage = (showPage % pages + pages) % pages;   
  int i = 0;
  for (auto& f : flights) {
    if (!f.icao) continue;
    if (i++ / rows != showPage) continue;   
    showSpr.printf("%-8s", f.callsign);
    if (f.flags & F_ALT)   showSpr.printf(" %5ld", (long)f.alt);   else showSpr.printf(" %5s", "");
    if (f.flags & F_TRK) showSpr.printf(" %3d", (int)f.track);   else showSpr.printf(" %3s", "");
    showSpr.printf(" %-10.10s %-10.10s\n", f.country ? f.country : "", f.airline ? f.airline : "");
  }
  // 一番下の行: 機体数とページ
  showSpr.setTextColor(C_PINK);
  showSpr.setCursor(0, LCD_H - 16);
  showSpr.printf("%d aircraft  page %d/%d", count, showPage + 1, pages);
  showSpr.pushSprite(0, 0);  
}


// ---- スリープ: 一覧を保存して deep sleep に入る。押し込みボタンを押すと起きる(起動し直す)
#define SHOW_LONG 1500   // 長押しと見なす時間 [ms]
void showSleep() {
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font0);
  lcd.setTextSize(2);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawString("sleep", LCD_W / 2, LCD_H / 2);
  flightsSave();
  while (digitalRead(SHOW_BTN) == LOW) delay(10);   
  delay(300);
  lcd.setBrightness(0);
  lcd.sleep();
  gpio_pullup_en((gpio_num_t)SHOW_BTN);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << SHOW_BTN, ESP_GPIO_WAKEUP_GPIO_LOW);   
  esp_deep_sleep_start();
}

// ボタンを短く押して離した瞬間なら true(長押しは net.h の Wi-Fi 受け取りに使うので無視)
bool showShort(int pin, uint32_t& down) {
  bool p = digitalRead(pin) == LOW, r = false;
  if (p && !down) down = millis();
  if (!p && down) { r = millis() - down < SHOW_LONG; down = 0; }
  return r;
}

void showLoop() {
  static uint32_t dL = 0, dR = 0;
  static uint32_t last = 0, down = 0;   // down: 押し込みボタンを押し始めた時刻(0 = 押していない)
  bool p = digitalRead(SHOW_BTN) == LOW;
  if (p && !down) down = millis();
  if (p && millis() - down > SHOW_LONG) showSleep();                 // 長押し: スリープ
  if (!p && down) { showMap = !showMap; last = 0; down = 0; }        // 短く押して離した: 地図 ↔ 一覧
#ifdef SW_L
  int d = showShort(SW_R, dR) - showShort(SW_L, dL);                 // 右 +1、左 -1(離したとき)
  if (d) { if (showMap) showSelect(d); else showPage += d; last = 0; }
#endif

  if (millis() - last < SHOW_INTERVAL) return;
  last = millis();
  if (showMap) showplaneonmap();
  else         showList();
}
