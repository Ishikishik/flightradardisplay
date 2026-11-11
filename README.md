# フライトレーダー表示プロジェクト
ご自宅で受信しているADS-Bの情報をいい感じの可愛いディスプレイに映す

# フォルダ構成と補足情報
## firm
- esp32に書き込む情報(arduino-cliで開発)
### 手順
1. 最初にmaketable.pyを使ってtable(地図データ、航空会社,国対応表、国,航空会社野画像)を作る
- 著作権の関係から画像は載っていません
1. secret.cppを以下のように追加
1. setting.cppをいい感じに編集
## pcb
- 銅箔の情報
- 単三2本かtype-cで給電(typec側を優先する回路付き)
## hard
- 外装
- カメラと同じネジで固定可能
## server
- ご自宅のラズパイが呟くadsb情報jsonをバイナリ形式にしてwebsocketで呟く(プロトコルは[こちら](/server/PROTOCOL.md))



# おせわになりました。
- [ICAOから航空会社、国を特定する部分](https://raw.githubusercontent.com/vradarserver/standing-data/main/)
  - Virtual Radar Server standing-data(CC0)
- [航空会社の画像](https://github.com/anhthang/soaring-symbols)
  - soaring-symbols(MIT)。尾翼マーク・社名ロゴの SVG
- [航空会社の画像(追加分)](https://github.com/Jxck-S/airline-logos)
  - Jxck-S/airline-logos。soaring-symbols にない会社を補完
- [国旗の画像](https://flagcdn.com)
  - flagcdn.com
- [地図(海岸線)](https://www.naturalearthdata.com)
  - Natural Earth(パブリックドメイン)のデータを cartopy で描画して作成
- [液晶の描画](https://github.com/lovyan03/LovyanGFX)
  - LovyanGFX
- [WebSocket(ESP32 側)](https://github.com/Links2004/arduinoWebSockets)
  - arduinoWebSockets
- [WebSocket(サーバー側)](https://github.com/python-websockets/websockets)
  - websockets(Python)

航空会社のロゴは各社の商標・著作物です。このリポジトリには含めていません(`firm/maketable/images/` は中身を git 管理外にしています)。必要な場合は `maketable.py --images` で各自取得してください。