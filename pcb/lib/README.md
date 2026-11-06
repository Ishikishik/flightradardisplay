# kawaeyes 部品ライブラリ

このプロジェクト専用の KiCad ライブラリです。
`pcb/sym-lib-table` と `pcb/fp-lib-table` から `${KIPRJMOD}/lib/...` で参照しています。

```
lib/
├── symbols/      回路図シンボル（1部品 = 1ファイル）
├── footprints/   フットプリント（<ライブラリ名>.pretty/）
└── 3dmodels/     3Dモデル（STEP）
```

| 部品 | 用途 | シンボル | フットプリント | 3D | 入手元 |
|---|---|---|---|---|---|
| TPS613221ADBVT | 昇圧DC-DC | ✓ | ✓ | ✓ | Component Search Engine |
| AP7333-33SAG-7 | 3.3V LDO | ✓ | ✓ | ✓ | Component Search Engine |
| LPWI252010S2R2T | インダクタ 2.2µH | ✓ | ✓ | ✓ | Component Search Engine |
| S-2110A (S-2110 AW K1) | ロータリーコードSW | ✓ | ✓ | ✓ | Component Search Engine |
| NJL7302L-F3 | 照度センサー | ✓ | ✓ | △ | シンボル・FP: Component Search Engine ／ 3D: データシート外形から作成した簡易モデル（外形の箱＋リード） |
| SSAJ120100 | スライドSW (ALPS) | ✓ | ✓ | ✓ | Component Search Engine |
| CH217K | 理想ダイオード | ✓ | ✓ | ✓ | Component Search Engine |
| TCK108AF | ロードスイッチ | ✓ | ✓ | ✓ | Component Search Engine |
| S-80142ALMC-JA3-T2 | 電圧検出器 | ✓ | ✓ | ✓ | Component Search Engine |
| XC8111AA01MR-G | ロードスイッチ (Torex) | ✓ | ✓ | ✓ | Component Search Engine |
| RT9080-33GJ5 | 3.3V LDO (Richtek) | ✓ | ✓ | ✓ | Component Search Engine |
| LS6J2M-T | タクトスイッチ | ✓ | ✓ | ✓ | Component Search Engine（ダウンロード済みライブラリから） |
| MSP2807 (ILI9341 2.8") | TFT液晶 | ✓（自作） | ✓（自作） | △ | FP: LCDwiki公式外形図 MSP2807_Size.pdf から作成（14ピン＋M3穴2つ＋外形。1番ピンから遠い側の2穴のみ）。3D: 同図面の簡易モデル（基板・液晶・ヘッダ、直挿し想定） |
| BH-321-2P | 電池ボックス 単3×2 | 標準 Device:Battery | ✓（自作） | △ | FP: メーカー図面(PCB/MOUNTING LAYOUT)から作成（取付穴なし）。1番=＋、2番=−。3D: 外形の箱＋ピン(13.4mm)の簡易モデル |
| TMHU26BR | レバー(ホイール)SW 秋月108070 (TMHU26) | ✓ | ✓ | ✓ | Component Search Engine。端子: 1, 2, T1(NO), C1(COM, 右タブ), E1(左タブ)。φ1.1 NPTH×2 |
| TMHU28 | レバー(ホイール)SW（採用予定、TMHU26BRから変更） | ✓ | ✓ | ✗ | Component Search Engine（3Dモデルなし）。8端子: 1,2,T1,3,4,C2(1.5mmピッチ)+側面タブE1,C1。φ1.2 NPTH×2。端子機能はデータシート要確認 |

## 部品を追加するとき

1. シンボルを `symbols/<部品名>.kicad_sym` に置く
2. フットプリントを `footprints/<部品名>.pretty/` に置く
3. 3Dモデルを `3dmodels/` に置き、フットプリントの model パスを
   `${KIPRJMOD}/lib/3dmodels/<ファイル名>` にする
4. KiCad の「設定 → シンボル/フットプリントライブラリの管理 → プロジェクト固有のライブラリ」に追加する
