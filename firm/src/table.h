

#define MAP_LINES 83      // 地図: 海岸線の線分 {x0, y0, x1, y1}(320x240、config.h の範囲)
#define COUNTRY_N 250     // 国名の数
#define COUNTRY_BLOCK_N 809  // ICAO アドレスの範囲の数
#define AIRLINE_N 5905    // 航空会社の数

// 国旗とロゴ: 16 色(4 bit/px)。1 バイトに 2 ピクセル(左が上位 4 ビット)、パレットは RGB565
#define FLAG_W 24
#define FLAG_H 16
#define FLAG_N 245        // 国旗の数
#define LOGO_W 16
#define LOGO_H 16
#define LOGO_N 165        // ロゴの数

struct CountryBlock { uint32_t start, end; uint8_t country; };     // 範囲 → COUNTRY_NAME の番号
struct Airline { char code[4]; const char* name; int16_t logo; };  // ICAO 3 文字コード → 名前、ロゴの番号(-1 = なし)

extern const int16_t  COUNTRY_FLAG[COUNTRY_N];                  // 国 → 国旗の番号(-1 = なし)
extern const uint16_t FLAG_PAL[FLAG_N][16];
extern const uint8_t  FLAG_PIX[FLAG_N][FLAG_W * FLAG_H / 2];
extern const uint16_t LOGO_PAL[LOGO_N][16];
extern const uint8_t  LOGO_PIX[LOGO_N][LOGO_W * LOGO_H / 2];

extern const int16_t mapLines[MAP_LINES][4];
extern const char* const COUNTRY_NAME[COUNTRY_N];
extern const CountryBlock COUNTRY_BLOCK[COUNTRY_BLOCK_N];   // 狭い範囲から順
extern const Airline AIRLINE[AIRLINE_N];                   // コード順(二分探索用)
