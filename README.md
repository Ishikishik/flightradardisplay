# フライトレーダー表示プロジェクト

ESP32-C3 と小型液晶ディスプレイを使い、**フライトレーダーのフィードを表示するデバイス**です。  
ラズベリーパイを使った例よりも、**安価かつコンパクト**に構築できます。

---

## 使用部品

| 部品 | 説明・購入先 |
| ---- | ----------- |
| [ESP32-C3](https://akizukidenshi.com/catalog/g/g117493/) | メインマイコン |
| [1.54インチ TFT 液晶](https://akizukidenshi.com/catalog/g/g116265/) | 地図・便名表示用 |
| [USB Type-C コネクタ DIP 化キット](https://akizukidenshi.com/catalog/g/g115426/) | 電源・書き込み用 |
| コンデンサ / 抵抗 / 3端子レギュレータ | 電源安定化などに適宜使用 |

---

## コード構成

### 1. 地図生成ツール(makemap)
- **`makemap.py`**  
  指定した緯度・経度範囲の白黒地図を生成します。  
  ご自分の地域に合わせてパラメータを設定してください。
- **`tolist.py`**  
  生成した白黒地図を **LCD に描画しやすい線形データ** に変換します。

### 2. 本体コード `kawaeyes`
- **`kawaeyes.ino`**  
  ESP32-C3 で動作するメインプログラム。
- **`library.cpp`**  
  - `setupwifi()`：Wi-Fi 接続処理  
  - `showjson()`：飛行機の位置と便名を地図上に表示  
  - `showjson2()`：便名と位置を文字のみで表示
- **`mapdata.cpp`**  
  東京都調布市（カワウソの住処）を例に `makemap` で生成した地図データ。

- **`secret.hpp`**  
  **Wi-Fi の SSID / パスワード / フィーダーの IP を定義するため、作成してください。**  
  下記の例を参考に、自分の環境に合わせて内容を編集します。

  ```cpp
  #ifndef SECRET_HPP
  #define SECRET_HPP

  #define SSID ""    // Wi-Fi SSID
  #define PASS ""    // Wi-Fi パスワード
  #define IP   ""    // フィーダーの IP
                          // 例: http://<IP>:8754/flights.json
                          // もしエンドポイントが異なる場合は
                          // getjson.cpp 内の showjson() を調整してください

  #endif

---

## 参考リンク
- [LovyanGFX ライブラリ（LCD制御）](https://github.com/lovyan03/LovyanGFX)
- [ESP32 書き込み手順 (Qiita)](https://qiita.com/McbeEringi/items/b791399b03aba0929368)

---

## 使い方

1. **地図データ作成**  
   `makemap.py` で範囲を指定して白黒地図を生成 → `tolist.py` で LCD 用データに変換。

2. **ESP32-C3 へ書き込み**  
   生成した `mapdata.cpp` をプロジェクトに追加し、`kawaeyes.ino` をビルド＆書き込み。

3. **起動**  
   電源投入後、自動的に Wi-Fi へ接続し、フライトレーダーのフィードを取得して地図上に便名と位置を表示します。
