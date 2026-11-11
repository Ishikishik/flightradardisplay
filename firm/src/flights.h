

#pragma pack(push, 1)   // 届く 1 機分(24 バイト)。先頭には 4 バイトのヘッダが付く
struct KeAircraft { uint8_t icao[3], flags; int32_t lat_e5, lon_e5; int16_t alt_25ft; uint8_t category, track; char callsign[8]; };
#pragma pack(pop)
enum { F_POS = 1, F_ALT = 2, F_CS = 4, F_GND = 8, F_TRK = 16 };   // flags: 何が有効か

#define FLIGHT_MAX 100
#define TRAIL_LEN  32     // 過去の位置の点数
#define TRAIL_STEP 0.05f  // この距離 [度](約 5km、画面で 2 ピクセルくらい)動くごとに 1 点残す
struct Pos { float lat, lon; };

struct Flight {
  uint32_t icao;                   // 0 = 空き
  uint8_t  flags, category;
  char     callsign[9];
  Pos      pos, prev;  int32_t alt; // 今と 1 つ前に届いた位置 [度]、高度 [ft]
  float    track;                  // [度] 北 0、時計回り
  const char *country, *airline;   // 分からなければ nullptr
  int16_t  flag, logo;             // table.h の国旗とロゴの番号(-1 = なし)
  Pos      trail[TRAIL_LEN];       // 過去の位置(リングバッファ)
  uint8_t  trailHead, trailN;
  uint32_t posTime, posDt;         // 最後に動いた時刻と、その前に動いてからの間隔 [ms](show.h の補間用)
};
Flight flights[FLIGHT_MAX];
bool   flightsNew = false;         // 更新されたら true
// 過去の位置を古い順に取り出す(i = 0 が一番古い)
Pos flightsTrail(const Flight& f, int i) { return f.trail[(f.trailHead + TRAIL_LEN - f.trailN + i) % TRAIL_LEN]; }

void flightsSetCountry(Flight& f) {   // ICAO アドレス → 国名と国旗。狭い範囲から並んでいるので最初に当たったもの
  f.country = nullptr; f.flag = -1;
  for (auto& b : COUNTRY_BLOCK)
    if (b.start <= f.icao && f.icao <= b.end) { f.country = COUNTRY_NAME[b.country]; f.flag = COUNTRY_FLAG[b.country]; return; }
}
void flightsSetAirline(Flight& f) {   // コールサインの先頭 3 文字 → 航空会社名とロゴ(二分探索)
  auto* a = f.callsign[0] ? (const Airline*)bsearch(f.callsign, AIRLINE, AIRLINE_N, sizeof(Airline),
    [](const void* k, const void* e) { return strncmp((const char*)k, ((const Airline*)e)->code, 3); }) : nullptr;
  f.airline = a ? a->name : nullptr;
  f.logo    = a ? a->logo : -1;
}
void flightsDecode(const uint8_t* data, size_t len) {
  if (len < 4) return;
  int n = min((int)(data[2] | data[3] << 8), (int)(len - 4) / 24);   // ヘッダの機体数
  bool seen[FLIGHT_MAX] = {};
  for (int i = 0; i < n; ++i) {
    KeAircraft a;
    memcpy(&a, data + 4 + i * 24, 24);
    uint32_t icao = a.icao[0] | a.icao[1] << 8 | a.icao[2] << 16;
    int k = -1;   // 同じ機体、なければ空き
    for (int j = 0; j < FLIGHT_MAX; ++j) {
      if (flights[j].icao == icao) { k = j; break; }
      if (!flights[j].icao && k < 0) k = j;
  }
    if (k < 0) continue;   // 満杯
    Flight& f = flights[k];
    if (f.icao != icao) { memset(&f, 0, sizeof(f)); f.icao = icao; flightsSetCountry(f); f.logo = -1; }
    seen[k] = true;
    f.flags = a.flags, f.category = a.category, f.alt = a.alt_25ft * 25, f.track = a.track * 360.0f / 256;
    if (a.flags & F_CS) {
      memcpy(f.callsign, a.callsign, 8);
      for (int j = 7; j >= 0 && f.callsign[j] == ' '; --j) f.callsign[j] = 0;
      flightsSetAirline(f);
    }
    Pos p = {a.lat_e5 / 1e5f, a.lon_e5 / 1e5f};
    if ((a.flags & F_POS) && (!f.posTime || p.lat != f.pos.lat || p.lon != f.pos.lon)) {   // 動いたとき
      uint32_t now = millis();
      f.posDt = f.posTime ? constrain(now - f.posTime, 250UL, 3000UL) : 1000;
      f.prev = f.posTime ? f.pos : p;   // show.h は prev → pos の間を補間して描く
      f.pos = p;
      f.posTime = now;
      Pos l = f.trailN ? flightsTrail(f, f.trailN - 1) : p;
      if (!f.trailN || fabsf(p.lat - l.lat) + fabsf(p.lon - l.lon) >= TRAIL_STEP) {
        f.trail[f.trailHead] = p;
        f.trailHead = (f.trailHead + 1) % TRAIL_LEN;
        if (f.trailN < TRAIL_LEN) ++f.trailN;
      }
    }
  }
  for (int k = 0; k < FLIGHT_MAX; ++k) if (!seen[k]) flights[k].icao = 0;
  flightsNew = true;
}

// ---- キャッシュ: 電源を切る前の一覧(軌跡ごと)をフラッシュ(LittleFS)に保存し、次の起動で引き継ぐ
#define FLIGHTS_CACHE "/flights.bin"
#define FLIGHTS_MAGIC 0x4B454631   // "KEF1"。形が変わったら古いキャッシュは読まない

void flightsSave() {
  if (!LittleFS.begin(true)) return;
  File file = LittleFS.open(FLIGHTS_CACHE, "w");
  uint32_t head[2] = {FLIGHTS_MAGIC, sizeof(Flight)};
  file.write((const uint8_t*)head, sizeof(head));
  file.write((const uint8_t*)flights, sizeof(flights));
  file.close();
}

void flightsLoad() {
  if (!LittleFS.begin(true) || !LittleFS.exists(FLIGHTS_CACHE)) return;
  File file = LittleFS.open(FLIGHTS_CACHE, "r");
  uint32_t head[2];
  bool ok = file.read((uint8_t*)head, sizeof(head)) == sizeof(head) && head[0] == FLIGHTS_MAGIC && head[1] == sizeof(Flight)
         && file.read((uint8_t*)flights, sizeof(flights)) == sizeof(flights);
  file.close();
  if (!ok) { memset(flights, 0, sizeof(flights)); return; }
  for (auto& f : flights) {
    if (!f.icao) continue;
    flightsSetCountry(f);
    flightsSetAirline(f);
    f.posTime = 0; f.posDt = 1000;  
  }
}
