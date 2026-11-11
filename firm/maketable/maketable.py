#!/usr/bin/env python3
"""ファームで使う表をまとめて src/table.h(形と件数)と src/table.cpp(中身)に書き出す。

  1. 地図   海岸線の線分(cartopy で描いた地図から輪郭を取り出して直線に近似)
  2. 国     ICAO アドレスの範囲 → 国名
  3. 航空会社 ICAO 3 文字コード → 航空会社名
  4. 国旗とロゴ RECOMMEND の大きさ・16 色に減らした画像(先に --images で images/ に保存しておく)

地図の範囲は src/config.h の LON_MIN / LON_MAX / LAT_MIN / LAT_MAX を読む。
国と航空会社のデータ: Virtual Radar Server standing-data (CC0)
  https://github.com/vradarserver/standing-data

使い方(firm フォルダで):
  python3 maketable/maketable.py             # 国と航空会社は GitHub から最新を取ってくる
  python3 maketable/maketable.py --src DIR   # 手元に clone した standing-data を使う
  python3 maketable/maketable.py --images    # 国旗と航空会社ロゴを maketable/images/ に PNG で保存する(確認用)
                                             # 足りないロゴは maketable/images/logos_manual/ICAO.png(.svg)に置く(ダウンロード分より優先)
  python3 maketable/maketable.py --preview flags 36x24 16   # 国旗を 36x24・16 色にした一覧を maketable/preview/ に出す
  python3 maketable/maketable.py --preview logos 32x32 16   # ロゴも同じように
  python3 maketable/maketable.py --preview logos            # 大きさと色数を省くとおすすめの値
必要なもの: pip install matplotlib cartopy opencv-python numpy
  --images だけ追加で: pip install cairosvg(Mac は先に brew install cairo)
"""
import argparse
import csv
import io
import re
import unicodedata
import urllib.request
from pathlib import Path

import cartopy.crs as ccrs
import cartopy.feature as cfeature
import cv2
import matplotlib
matplotlib.use("Agg")   # 画面に出さない
import matplotlib.pyplot as plt
import numpy as np

FIRM = Path(__file__).resolve().parent.parent
W, H = 320, 240          # 地図の大きさ(液晶を横向きにしたとき)
EPSILON = 0.002          # 直線近似の細かさ(小さいほど線が増えて正確)

RAW = "https://raw.githubusercontent.com/vradarserver/standing-data/main/"
CSV = {
    "blocks": "code-blocks/schema-01/code-blocks.csv",
    "countries": "countries/schema-01/countries.csv",
    "airlines": "airlines/schema-01/airlines.csv",
}


# ---------------------------------------------------------------- 地図

def map_range():
    """config.h から地図の範囲を読む"""
    text = (FIRM / "src" / "config.h").read_text(encoding="utf-8")
    get = lambda k: float(re.search(rf"#define\s+{k}\s+([-\d.]+)", text).group(1))
    return get("LON_MIN"), get("LON_MAX"), get("LAT_MIN"), get("LAT_MAX")


def map_lines():
    lon_min, lon_max, lat_min, lat_max = map_range()
    fig = plt.figure(figsize=(4, 3), dpi=150)
    ax = plt.axes(projection=ccrs.PlateCarree())
    ax.set_aspect("auto")
    ax.set_extent([lon_min, lon_max, lat_min, lat_max], crs=ccrs.PlateCarree())
    ax.axis("off")
    ax.add_feature(cfeature.LAND, facecolor="black")
    ax.add_feature(cfeature.OCEAN, facecolor="white")
    buf = io.BytesIO()
    fig.savefig(buf, format="png", bbox_inches="tight", pad_inches=0)
    plt.close(fig)

    img = cv2.imdecode(np.frombuffer(buf.getvalue(), np.uint8), cv2.IMREAD_GRAYSCALE)
    img = cv2.resize(img, (W, H))
    _, land = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY_INV)   # 陸地を白に
    contours, _ = cv2.findContours(land, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    c = max(contours, key=cv2.contourArea)                          # 一番大きな陸地
    pts = cv2.approxPolyDP(c, EPSILON * cv2.arcLength(c, True), True)[:, 0]
    return [(*pts[i], *pts[(i + 1) % len(pts)]) for i in range(len(pts))]


# ---------------------------------------------------------------- 国と航空会社

def load(src, key):
    if src:
        text = (Path(src) / CSV[key]).read_text(encoding="utf-8-sig")
    else:
        with urllib.request.urlopen(RAW + CSV[key], timeout=30) as r:
            text = r.read().decode("utf-8-sig")
    return list(csv.DictReader(io.StringIO(text)))


def cstr(s):
    """液晶のフォントは ASCII だけなので、アクセント記号などを落として C の文字列にする"""
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode().strip()
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


# ---------------------------------------------------------------- 書き出し

# ---------------------------------------------------------------- 国旗とロゴの画像(確認用に PNG で保存)
# 国旗: flagcdn.com(ISO 2 文字コード) / ロゴ: soaring-symbols(MIT。ロゴ自体は各社の商標なので公開しない)
IMAGES = FIRM / "maketable" / "images"
SOARING = "https://codeload.github.com/anhthang/soaring-symbols/zip/refs/heads/main"


def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "kawaeyes-maketable"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()


def download_images(src):
    import zipfile
    import cairosvg

    # 国旗: table.h と同じ国の一覧(ISO コード)で取ってくる
    out = IMAGES / "flags"
    out.mkdir(parents=True, exist_ok=True)
    ok = 0
    for c in load(src, "countries"):
        try:
            (out / f"{c['ISO']}.png").write_bytes(fetch(f"https://flagcdn.com/w80/{c['ISO'].lower()}.png"))
            ok += 1
        except Exception as e:
            print(f"  国旗なし: {c['ISO']} {c['Name']} ({e})")
    print(f"国旗: {ok} 枚 -> {out}")

    # ロゴ: マークだけの icon.svg を優先、なければ横長の logo.svg。ICAO コードのファイル名で保存
    out = IMAGES / "logos"
    out.mkdir(parents=True, exist_ok=True)
    z = zipfile.ZipFile(io.BytesIO(fetch(SOARING)))
    root = z.namelist()[0].split("/")[0]
    ok = 0
    for a in __import__("json").loads(z.read(f"{root}/airlines.json")):
        if not a.get("icao"):
            continue
        for kind in ("icon.svg", "logo.svg"):
            path = f"{root}/assets/{a['slug']}/{kind}"
            if path in z.namelist():
                cairosvg.svg2png(bytestring=z.read(path), write_to=str(out / f"{a['icao']}.png"), output_height=64)
                ok += 1
                break
    print(f"ロゴ: {ok} 枚 -> {out}")




# ---------------------------------------------------------------- 国旗とロゴのお試し変換(大きさと色数を決めるため)
PREVIEW = FIRM / "maketable" / "preview"

# 国旗とロゴの大きさと色数(table を作るときと、--preview で省いたときに使う)
# 情報欄の文字(8x16 フォント)の高さ 16 にそろえる。16 色 = 4 bit/px
RECOMMEND = {"flags": ((24, 16), 16), "logos": ((16, 16), 16)}


def image_files(kind):
    """kind("flags" / "logos")の画像を {コード: パス} で返す。

    logos は images/logos(ダウンロード)と images/logos_manual(手で置いたもの)を合わせ、
    同じ会社は手で置いたほうを使う。手で置いたものどうしは、更新日時が新しいほうを使う。
    """
    files = {f.stem.upper(): f for f in (IMAGES / kind).glob("*.png")}
    if kind == "logos":
        manual = sorted((IMAGES / "logos_manual").glob("*"), key=lambda f: f.stat().st_mtime)   # 古い順 → 新しいもので上書き
        files.update({f.stem.upper(): f for f in manual if f.suffix.lower() in (".png", ".jpg", ".jpeg", ".webp", ".svg")})
    return dict(sorted(files.items()))


def open_image(path):
    """PNG / JPEG / WebP / SVG を RGBA の画像として開く"""
    from PIL import Image
    if path.suffix.lower() == ".svg":
        import cairosvg
        return Image.open(io.BytesIO(cairosvg.svg2png(url=str(path), output_height=64))).convert("RGBA")
    return Image.open(path).convert("RGBA")


def shrink(kind, path, size, colors):
    """画像を size=(幅, 高さ)、colors 色に減らす(preview と table の両方で使う)。

    flags: 縦横比を変えて size にぴったり合わせる
    logos: 縦横比を保って size に収め、余白は黒(液晶の背景色)
    colors <= 256 ならパレット画像(mode "P")、それより多ければ RGB で返す。
    """
    from PIL import Image
    w, h = size
    src = open_image(path)
    if kind == "logos":
        src.thumbnail((w, h), Image.LANCZOS)
        img = Image.new("RGBA", (w, h), (0, 0, 0, 255))
        img.alpha_composite(src, ((w - src.width) // 2, (h - src.height) // 2))
        img = img.convert("RGB")
    else:
        img = src.convert("RGB").resize((w, h), Image.LANCZOS)
    if colors <= 256:
        img = img.quantize(colors=colors, dither=Image.Dither.NONE)   # 画像ごとに一番合う色を選ぶ
    return img


def pack4(img):
    """16 色のパレット画像 → (RGB565 のパレット 16 色, 1 バイトに 2 ピクセル詰めたピクセル)。左のピクセルが上位 4 ビット"""
    rgb = (img.getpalette() + [0] * 48)[:48]
    pal = [((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) for r, g, b in zip(rgb[0::3], rgb[1::3], rgb[2::3])]
    px = img.tobytes()
    return pal, bytes((px[i] << 4) | px[i + 1] for i in range(0, len(px), 2))


def preview(kind, size, colors):
    """images/{kind}/*.png を size=(幅, 高さ)、colors 色に減らした一覧画像を preview/ に出し、必要なバイト数を出す。

    kind = "flags": 縦横比を変えて size にぴったり合わせる
    kind = "logos": 縦横比を保って size に収め、余白は透明(一覧では黒 = 液晶の背景色で見せる)
    LovyanGFX で描ける形に合わせて、1 ピクセルのビット数は 1/2/4/8 に切り上げる。
    colors > 256 なら RGB565(パレットなし、2 バイト/ピクセル)として計算する。
    """
    from PIL import Image

    w, h = size
    bpp = 16 if colors > 256 else next(b for b in (1, 2, 4, 8) if colors <= 1 << b)
    palette_bytes = 0 if bpp == 16 else (1 << bpp) * 2      # RGB565 で 1 色 2 バイト
    pixel_bytes = (w * h * bpp + 7) // 8
    name = f"{kind}_{w}x{h}_{colors}c"
    PREVIEW.mkdir(parents=True, exist_ok=True)

    files = list(image_files(kind).values())
    zoom, cols = 4, 16                                        # 一覧は 4 倍に拡大して 16 列に並べる
    sheet = Image.new("RGB", (cols * (w + 2) * zoom, ((len(files) + cols - 1) // cols) * (h + 2) * zoom), "gray")
    for i, f in enumerate(files):
        img = shrink(kind, f, size, colors).convert("RGB")
        sheet.paste(img.resize((w * zoom, h * zoom), Image.NEAREST),
                    ((i % cols) * (w + 2) * zoom, (i // cols) * (h + 2) * zoom))
    sheet.save(PREVIEW / f"{name}.png")

    total = (palette_bytes + pixel_bytes) * len(files)
    print(f"{name}: {len(files)} 枚 / {bpp} bit/px")
    print(f"  1 枚 = パレット {palette_bytes} B + ピクセル {pixel_bytes} B = {palette_bytes + pixel_bytes} B")
    print(f"  合計 = {total} B ({total / 1024:.1f} KB)")
    print(f"  一覧: {PREVIEW / (name + '.png')}")
    return total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", help="手元の standing-data のフォルダ")
    ap.add_argument("--images", action="store_true", help="国旗とロゴをダウンロードして PNG で保存する")
    ap.add_argument("--preview", nargs="+", metavar="KIND [WxH COLORS]",
                    help="お試し変換 例: --preview flags 36x24 16。大きさと色数を省くとおすすめ(RECOMMEND)")
    args = ap.parse_args()
    if args.images:
        download_images(args.src)
        return
    if args.preview:
        kind, rest = args.preview[0], args.preview[1:]
        size, colors = RECOMMEND[kind]
        if rest:
            size, colors = tuple(map(int, rest[0].lower().split("x"))), int(rest[1])
        preview(kind, size, colors)
        return

    lines = map_lines()

    countries = load(args.src, "countries")
    index = {c["ISO"]: i for i, c in enumerate(countries)}
    blocks = [b for b in load(args.src, "blocks") if b["CountryISO2"] in index and b["CountryISO2"] != "ZZ"]
    blocks.sort(key=lambda b: int(b["Count"]))   # 狭い範囲から(最初に当たったものが一番詳しい)

    # 国旗とロゴ(RECOMMEND の大きさ、16 色 = 4 bit/px)
    (fw, fh), fc = RECOMMEND["flags"]
    (lw, lh), lc = RECOMMEND["logos"]
    assert fc <= 16 and lc <= 16, "table に入れる画像は 16 色まで(4 bit/px)"
    flag_files = image_files("flags")
    if not flag_files:
        raise SystemExit("国旗がありません。先に python3 maketable/maketable.py --images を実行してください")
    flags = [(c["ISO"], *pack4(shrink("flags", flag_files[c["ISO"]], (fw, fh), fc)))
             for c in countries if c["ISO"] in flag_files]
    flag_no = {iso: i for i, (iso, _, _) in enumerate(flags)}

    airlines = {}
    for a in load(args.src, "airlines"):
        code = a["ICAO"].strip().upper()
        if len(code) == 3 and code.isalpha() and code not in airlines:
            airlines[code] = a["Name"]

    logo_files = image_files("logos")
    logos = [(code, *pack4(shrink("logos", logo_files[code], (lw, lh), lc))) for code in sorted(airlines) if code in logo_files]
    logo_no = {code: i for i, (code, _, _) in enumerate(logos)}
    unused = sorted(set(logo_files) - set(airlines))   # 航空会社の表にないコードのロゴ(ファイル名の間違いなど)

    def arr(data):   # バイト列 → C の配列の中身
        return ", ".join(f"0x{b:02X}" for b in data)

    def pal(p):
        return ", ".join(f"0x{v:04X}" for v in p)

    head = [
        "// maketable/maketable.py で自動生成。手で書き換えない",
        "// 国と航空会社のデータ: Virtual Radar Server standing-data (CC0)",
        "// 国旗: flagcdn.com / ロゴ: soaring-symbols、Jxck-S/airline-logos、手で置いたもの(各社の商標)",
    ]
    # table.h: 形と件数だけ(main.cpp から include する)
    h = [
        "// table.h  前提: なし",
        *head,
        "// 中身は table.cpp。大きいデータを別ファイルにしておくと、main.cpp を直したときに作り直さずに済む",
        "",
        f"#define MAP_LINES {len(lines)}      // 地図: 海岸線の線分 {{x0, y0, x1, y1}}({W}x{H}、config.h の範囲)",
        f"#define COUNTRY_N {len(countries)}     // 国名の数",
        f"#define COUNTRY_BLOCK_N {len(blocks)}  // ICAO アドレスの範囲の数",
        f"#define AIRLINE_N {len(airlines)}    // 航空会社の数",
        "",
        "// 国旗とロゴ: 16 色(4 bit/px)。1 バイトに 2 ピクセル(左が上位 4 ビット)、パレットは RGB565",
        f"#define FLAG_W {fw}",
        f"#define FLAG_H {fh}",
        f"#define FLAG_N {len(flags)}        // 国旗の数",
        f"#define LOGO_W {lw}",
        f"#define LOGO_H {lh}",
        f"#define LOGO_N {len(logos)}        // ロゴの数",
        "",
        "struct CountryBlock { uint32_t start, end; uint8_t country; };     // 範囲 → COUNTRY_NAME の番号",
        "struct Airline { char code[4]; const char* name; int16_t logo; };  // ICAO 3 文字コード → 名前、ロゴの番号(-1 = なし)",
        "",
        "extern const int16_t  COUNTRY_FLAG[COUNTRY_N];                  // 国 → 国旗の番号(-1 = なし)",
        "extern const uint16_t FLAG_PAL[FLAG_N][16];",
        "extern const uint8_t  FLAG_PIX[FLAG_N][FLAG_W * FLAG_H / 2];",
        "extern const uint16_t LOGO_PAL[LOGO_N][16];",
        "extern const uint8_t  LOGO_PIX[LOGO_N][LOGO_W * LOGO_H / 2];",
        "",
        "extern const int16_t mapLines[MAP_LINES][4];",
        "extern const char* const COUNTRY_NAME[COUNTRY_N];",
        "extern const CountryBlock COUNTRY_BLOCK[COUNTRY_BLOCK_N];   // 狭い範囲から順",
        "extern const Airline AIRLINE[AIRLINE_N];                   // コード順(二分探索用)",
        "",
    ]
    # table.cpp: 中身(PlatformIO が別にコンパイルする)
    c = [
        "// table.cpp",
        *head,
        "#include <stdint.h>",
        '#include "table.h"',
        "",
        "const int16_t mapLines[MAP_LINES][4] = {",
        *[f"  {{{x0}, {y0}, {x1}, {y1}}}," for x0, y0, x1, y1 in lines],
        "};",
        "",
        "const char* const COUNTRY_NAME[COUNTRY_N] = {",
        *[f"  {cstr(c['Name'])},  // {i} {c['ISO']}" for i, c in enumerate(countries)],
        "};",
        "",
        "const CountryBlock COUNTRY_BLOCK[COUNTRY_BLOCK_N] = {",
        *[f"  {{0x{b['Start']}, 0x{b['Finish']}, {index[b['CountryISO2']]}}}," for b in blocks],
        "};",
        "",
        "const Airline AIRLINE[AIRLINE_N] = {",
        *[f'  {{"{c}", {cstr(airlines[c])}, {logo_no.get(c, -1)}}},' for c in sorted(airlines)],
        "};",
        "",
        "const int16_t COUNTRY_FLAG[COUNTRY_N] = {",
        *[f"  {flag_no.get(c['ISO'], -1)},  // {c['ISO']}" for c in countries],
        "};",
        "",
        "const uint16_t FLAG_PAL[FLAG_N][16] = {",
        *[f"  {{{pal(p)}}},  // {iso}" for iso, p, _ in flags],
        "};",
        "const uint8_t FLAG_PIX[FLAG_N][FLAG_W * FLAG_H / 2] = {",
        *[f"  {{{arr(px)}}},  // {iso}" for iso, _, px in flags],
        "};",
        "",
        "const uint16_t LOGO_PAL[LOGO_N][16] = {",
        *[f"  {{{pal(p)}}},  // {code}" for code, p, _ in logos],
        "};",
        "const uint8_t LOGO_PIX[LOGO_N][LOGO_W * LOGO_H / 2] = {",
        *[f"  {{{arr(px)}}},  // {code}" for code, _, px in logos],
        "};",
        "",
    ]
    (FIRM / "src" / "table.h").write_text("\n".join(h), encoding="utf-8")
    (FIRM / "src" / "table.cpp").write_text("\n".join(c), encoding="utf-8")
    print(f"table.h / table.cpp: 地図 {len(lines)} 本 / 国 {len(countries)}(範囲 {len(blocks)}) / 航空会社 {len(airlines)} 社")
    img_bytes = len(flags) * (32 + fw * fh // 2) + len(logos) * (32 + lw * lh // 2)
    print(f"  国旗 {len(flags)} 枚({fw}x{fh}) / ロゴ {len(logos)} 枚({lw}x{lh}) / 画像 合計 {img_bytes / 1024:.1f} KB")
    if unused:
        print(f"  航空会社の表にないため使わなかったロゴ: {' '.join(unused)}")


if __name__ == "__main__":
    main()
