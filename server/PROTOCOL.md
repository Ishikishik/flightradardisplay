# flightradardisplay配信プロトコルver1

WebSocket のバイナリメッセージ 1 つに、その時点の全機体を入れて 1 秒ごとに送る。
数値はすべてリトルエンディアン。

```
[ヘッダ 4B][機体レコード 24B] × count
```

接続 URL: `wss://<ホスト>/?token=<トークン>`(`&format=json` で同じ内容を JSON で受信できる。デバッグ用)

## ヘッダ(4 バイト)

| オフセット | 型 | 項目 | 内容 |
|---|---|---|---|
| 0 | uint8 | version | `1` |
| 1 | uint8 | type | `1` = 機体一覧 |
| 2 | uint16 | count | 機体数 |

## 機体レコード(24 バイト)

| オフセット | 型 | 項目 | 内容 |
|---|---|---|---|
| 0 | uint8[3] | icao | ICAO アドレス。`icao = b[0] \| b[1]<<8 \| b[2]<<16` |
| 3 | uint8 | flags | 下表 |
| 4 | int32 | lat_e5 | 緯度 × 10⁵ |
| 8 | int32 | lon_e5 | 経度 × 10⁵ |
| 12 | int16 | alt_25ft | 気圧高度 ÷ 25 ft |
| 14 | uint8 | category | 機体カテゴリー。`0xA3` = A3、`0x00` = 不明 |
| 15 | uint8 | track | 進行方向。360° を 256 段階、北 0 で時計回り。`度 = track × 360 / 256` |
| 16 | char[8] | callsign | コールサイン(ASCII、空きは空白) |

### flags

| ビット | 意味 |
|---|---|
| 0 | 位置が有効 |
| 1 | 高度が有効 |
| 2 | コールサインが有効 |
| 3 | 地上にいる |
| 4 | 進行方向が有効 |
| 5〜7 | 予備(0) |

無効な項目の値は 0。60 秒以上受信がない機体はサーバー側で除外する。

## ESP32 側の構造体

```c
#pragma pack(push, 1)
struct KeHeader {
  uint8_t  version;
  uint8_t  type;
  uint16_t count;
};
struct KeAircraft {
  uint8_t  icao[3];
  uint8_t  flags;
  int32_t  lat_e5;
  int32_t  lon_e5;
  int16_t  alt_25ft;
  uint8_t  category;
  uint8_t  track;
  char     callsign[8];
};
#pragma pack(pop)
static_assert(sizeof(KeAircraft) == 24, "size mismatch");
```
