#!/usr/bin/env python3
"""kawaeyes ADS-B WebSocket サーバー

dump1090 が書き出す aircraft.json を一定間隔で読み、PROTOCOL.md の
バイナリ形式に詰め替えて、接続中のクライアントへ配信する。
解読は dump1090 に任せ、このサーバーは形を変えて中継するだけ。

接続:  wss://<公開ホスト名>/?token=<トークン>
       (または Authorization: Bearer <トークン> ヘッダ)
       デバッグ用に &format=json を付けると同じ内容を JSON で受け取れる。

設定は環境変数(systemd の EnvironmentFile で .env を読む想定):
  KAWAEYES_TOKENS  許可するトークン。カンマ区切りで複数可(必須)
  WS_HOST / WS_PORT  待ち受け先(既定 127.0.0.1:8765)
  AIRCRAFT_JSON      aircraft.json の場所。ファイルパスか http(s) URL
                     (既定 http://localhost/dump1090/data/aircraft.json)
  PUSH_INTERVAL      配信間隔[秒](既定 1.0)
"""
import asyncio
import hmac
import json
import logging
import os
import struct
import urllib.request
from urllib.parse import parse_qs, urlparse

import websockets

WS_HOST = os.environ.get("WS_HOST", "127.0.0.1")
WS_PORT = int(os.environ.get("WS_PORT", "8765"))
AIRCRAFT_JSON = os.environ.get("AIRCRAFT_JSON", "http://localhost/dump1090/data/aircraft.json")
PUSH_INTERVAL = float(os.environ.get("PUSH_INTERVAL", "1.0"))
TOKENS = [t.strip() for t in os.environ.get("KAWAEYES_TOKENS", "").split(",") if t.strip()]

STALE_SEC = 60      # この秒数以上受信がない機体は送らない
MAX_CLIENTS = 20    # 同時接続数の上限

# ---- バイナリ形式(詳細は PROTOCOL.md)
PROTOCOL_VERSION = 1
MSG_AIRCRAFT_LIST = 1
HEADER = struct.Struct("<BBH")          # version, type, count            = 4 B
RECORD = struct.Struct("<3sBiihBB8s")   # icao, flags, lat, lon, alt, cat, track, callsign = 24 B
assert HEADER.size == 4 and RECORD.size == 24

FLAG_POS = 1 << 0
FLAG_ALT = 1 << 1
FLAG_CALLSIGN = 1 << 2
FLAG_GROUND = 1 << 3
FLAG_TRACK = 1 << 4

log = logging.getLogger("kawaeyes")
clients: dict = {}   # websocket -> "bin" | "json"


# ---------------------------------------------------------------- 読み込みと変換

def _load_aircraft_json() -> dict:
    if AIRCRAFT_JSON.startswith(("http://", "https://")):
        with urllib.request.urlopen(AIRCRAFT_JSON, timeout=3) as r:
            return json.load(r)
    with open(AIRCRAFT_JSON, "rb") as f:
        return json.load(f)


def _category_byte(cat) -> int:
    """'A3' -> 0xA3。不明なら 0。"""
    if isinstance(cat, str) and len(cat) == 2:
        try:
            return int(cat, 16)
        except ValueError:
            pass
    return 0


def convert(data: dict) -> list[dict]:
    """aircraft.json を、送る項目だけの扱いやすい形に変換する。"""
    out = []
    for a in data.get("aircraft", []):
        hexid = a.get("hex", "")
        if hexid.startswith("~"):       # ICAO アドレスでない(TIS-B など)は除外
            continue
        try:
            icao = int(hexid, 16)
        except ValueError:
            continue
        if a.get("seen", 999) > STALE_SEC:
            continue

        flags = 0
        lat = lon = alt = track = 0.0
        if "lat" in a and "lon" in a and a.get("seen_pos", 999) <= STALE_SEC:
            flags |= FLAG_POS
            lat, lon = a["lat"], a["lon"]
        altitude = a.get("altitude", a.get("alt_baro"))
        if altitude == "ground":
            flags |= FLAG_GROUND
        elif isinstance(altitude, (int, float)):
            flags |= FLAG_ALT
            alt = altitude
        if isinstance(a.get("track"), (int, float)):
            flags |= FLAG_TRACK
            track = a["track"]
        callsign = (a.get("flight") or "").strip()
        if callsign:
            flags |= FLAG_CALLSIGN

        out.append({
            "icao": icao, "flags": flags, "lat": lat, "lon": lon, "alt": alt,
            "category": _category_byte(a.get("category")), "track": track,
            "callsign": callsign,
        })
    return out


def _clamp(v: int, lo: int, hi: int) -> int:
    return max(lo, min(hi, v))


def encode_binary(items: list[dict]) -> bytes:
    items = items[:0xFFFF]
    buf = bytearray(HEADER.pack(PROTOCOL_VERSION, MSG_AIRCRAFT_LIST, len(items)))
    for it in items:
        buf += RECORD.pack(
            it["icao"].to_bytes(3, "little"),
            it["flags"],
            round(it["lat"] * 1e5),
            round(it["lon"] * 1e5),
            _clamp(round(it["alt"] / 25), -32768, 32767),
            it["category"],
            round(it["track"] * 256 / 360) % 256,
            it["callsign"].encode("ascii", "replace")[:8].ljust(8, b" "),
        )
    return bytes(buf)


def encode_json(items: list[dict]) -> str:
    """デバッグ用。バイナリと同じ内容を人が読める形で。"""
    rows = []
    for it in items:
        f = it["flags"]
        rows.append({
            "icao": f"{it['icao']:06X}",
            "callsign": it["callsign"] if f & FLAG_CALLSIGN else None,
            "lat": it["lat"] if f & FLAG_POS else None,
            "lon": it["lon"] if f & FLAG_POS else None,
            "alt": it["alt"] if f & FLAG_ALT else None,
            "ground": bool(f & FLAG_GROUND),
            "track": it["track"] if f & FLAG_TRACK else None,
            "category": f"{it['category']:02X}" if it["category"] else None,
        })
    return json.dumps({"count": len(rows), "aircraft": rows}, separators=(",", ":"))


# ---------------------------------------------------------------- 配信

async def _send(ws, msg) -> None:
    try:
        await asyncio.wait_for(ws.send(msg), timeout=5)
    except Exception:
        pass  # 切断済みや遅いクライアントは今回は飛ばす(後始末は handler 側)


async def broadcaster() -> None:
    last_error = None
    while True:
        await asyncio.sleep(PUSH_INTERVAL)
        if not clients:
            continue
        try:
            items = convert(await asyncio.to_thread(_load_aircraft_json))
            last_error = None
        except Exception as e:
            if str(e) != last_error:            # 同じエラーを毎秒ログに出さない
                log.warning("aircraft.json を読めません: %s", e)
                last_error = str(e)
            continue
        binary = jsontext = None
        sends = []
        for ws, fmt in list(clients.items()):
            if fmt == "json":
                jsontext = jsontext or encode_json(items)
                sends.append(_send(ws, jsontext))
            else:
                binary = binary or encode_binary(items)
                sends.append(_send(ws, binary))
        await asyncio.gather(*sends)


# ---------------------------------------------------------------- 接続処理

def _request_path(ws) -> str:
    req = getattr(ws, "request", None)          # websockets 13 以降
    if req is not None and getattr(req, "path", None):
        return req.path
    return getattr(ws, "path", "") or ""        # websockets 10〜12(legacy)


def _request_headers(ws):
    req = getattr(ws, "request", None)
    if req is not None and getattr(req, "headers", None) is not None:
        return req.headers
    return getattr(ws, "request_headers", {})


def token_ok(ws, query: dict) -> bool:
    candidate = query.get("token", [""])[0]
    if not candidate:
        auth = _request_headers(ws).get("Authorization", "")
        if auth.startswith("Bearer "):
            candidate = auth[len("Bearer "):]
    if not candidate:
        return False
    return any(hmac.compare_digest(candidate.encode(), t.encode()) for t in TOKENS)


async def handler(ws, path=None) -> None:
    peer = _request_headers(ws).get("CF-Connecting-IP") or ws.remote_address
    query = parse_qs(urlparse(_request_path(ws)).query)
    if not token_ok(ws, query):
        log.warning("認証失敗: %s", peer)
        await ws.close(code=1008, reason="unauthorized")
        return
    if len(clients) >= MAX_CLIENTS:
        await ws.close(code=1013, reason="too many clients")
        return

    fmt = "json" if query.get("format", [""])[0] == "json" else "bin"
    clients[ws] = fmt
    log.info("接続: %s [%s] (計 %d)", peer, fmt, len(clients))
    try:
        async for _ in ws:      # クライアントからのメッセージは今は無視
            pass
    except websockets.ConnectionClosed:
        pass
    finally:
        clients.pop(ws, None)
        log.info("切断: %s (計 %d)", peer, len(clients))


async def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    if not TOKENS:
        log.error("KAWAEYES_TOKENS が設定されていません")
        raise SystemExit(1)
    log.info("websockets %s / 待ち受け %s:%d / データ元 %s",
             websockets.__version__, WS_HOST, WS_PORT, AIRCRAFT_JSON)
    async with websockets.serve(
        handler, WS_HOST, WS_PORT,
        max_size=4096,       # クライアントから受け取る最大サイズ
        ping_interval=20,    # Cloudflare の無通信切断対策
        ping_timeout=20,
    ):
        await broadcaster()


if __name__ == "__main__":
    asyncio.run(main())
