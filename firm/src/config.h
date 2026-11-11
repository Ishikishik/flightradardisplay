// ---- 基板ごとの違い
#if REV==1
	#define LCD_PANEL Panel_ILI9341
	#define LCD_PANEL_W 240 // パネル本来の大きさ
	#define LCD_PANEL_H 320
	#define LCD_ROT 3       // 横向き
	#define LCD_W 320       // 回転後
	#define LCD_H 240
	#define SW_A 0
#elif REV==2
	#define LCD_PANEL Panel_ILI9341 // v1 と同じ 2.8インチ 240x320(HSD028561-G2)
	#define LCD_PANEL_W 240
	#define LCD_PANEL_H 320
	#define LCD_ROT 3       // 横向き
	#define LCD_W 320       // 回転後
	#define LCD_H 240
	#define LCD_BL 5
	#define SW_L 10 // test.ino に合わせた。実機で要確認
	#define SW_R 0
	#define SW_P 1
	#define ALS_PWR 3
	#define ALS 4
#else
	#error "REV は 1 か 2 を指定してください"
#endif


// ---- 液晶(共通)
#define LCD_SCLK 6
#define LCD_MOSI 7
#define LCD_DC 9
#define LCD_RST 8

// ---- 地図の範囲(左右=経度、上下=緯度)
#define LON_MIN 135.755f
#define LON_MAX 142.954f
#define LAT_MIN 33.451f
#define LAT_MAX 39.865f

// ---- アンテナの位置
#define LON_HOME 139.529f
#define LAT_HOME 35.651f
