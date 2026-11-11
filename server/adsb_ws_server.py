#!/usr/bin/env python3
"""kawaeyes ADS-B WebSocket サーバー(試作版)

dump1090 の SBS(BaseStation)出力 (localhost:30003) を読み、機体ごとの最新状態を
ICAO をキーにして保持し、接続中のクライアントへ一定間隔で JSON を配信する。

接続:  wss://<公開ホスト名>/?token=<トークン>
       (または Authorization: Bearer <トークン> ヘッダ)

設定は環境変数(systemd の EnvironmentFile で .env を読む想定):
  KAWAEYES_TOKENS  許可するトークン。カンマ区切りで複数可(必須)
  WS_HOST / WS_PORT        待ち受け先(既定 127.0.0.1:8765)
  SBS_HOST / SBS_PORT      dump1090 の SBS 出力(既定 127.0.0.1:30003)
  PUSH_INTERVAL            配信間隔[秒](既定 1.0)
"""
import asyncio
import hmac
import json
import logging
import os
import time
from urllib.parse import parse_qs, urlparse

import websockets

SBS_HOST = os.environ.get("SBS_HOST", "127.0.0.1")
SBS_PORT = int(os.environ.get("SBS_PORT", "30003"))
WS_HOST = os.environ.get("WS_HOST", "127.0.0.1")
WS_PORT = int(os.environ.get("WS_PORT", "8765"))
PUSH_INTERVAL = float(os.environ.get("PUSH_INTERVAL", "1.0"))
TOKENS = [t.strip() for t in os.environ.get("KAWAEYES_TOKENS", "").split(",") if t.strip()]

STALE_SEC = 60      # この秒数受信がない機体は消す
MAX_CLIENTS = 20    # 同時接続数の上限

log = logging.getLogger("kawaeyes")

aircraft: dict[str, dict] = {}   # ICAO(16進6桁) -> 状態
clients: set = set()


# ---------------------------------------------------------------- SBS 受信

def _num(text: str, conv):
    text = text.strip()
    if not text:
        return None
    try:
        return conv(text)
    except ValueError:
        return None


def _int(text: str) -> int:
    return int(float(text))


# SBS の列番号 -> (保存するキー, 変換関数)
SBS_FIELDS = (
    ("alt", 11, _int),      # 高度 [ft]
    ("gs", 12, float),      # 対地速度 [kt]
    ("track", 13, float),   # 進行方向 [deg]
    ("lat", 14, float),
    ("lon", 15, float),
    ("vr", 16, _int),       # 昇降率 [ft/min]
)


def handle_sbs_line(line: str) -> None:
    p = line.split(",")
    if len(p) < 22 or p[0] != "MSG":
        return
    icao = p[4].strip().upper()
    if not icao:
        return

    now = time.time()
    a = aircraft.setdefault(icao, {"icao": icao})
    a["_seen"] = now

    callsign = p[10].strip()
    if callsign:
        a["callsign"] = callsign

    for key, idx, conv in SBS_FIELDS:
        v = _num(p[idx], conv)
        if v is not None:
            a[key] = v
    if p[14].strip() and p[15].strip():
        a["_pos_seen"] = now

    squawk = p[17].strip()
    if squawk:
        a["squawk"] = squawk
    ground = p[21].strip()
    if ground:
        a["ground"] = ground in ("-1", "1")


async def sbs_reader() -> None:
    """SBS ポートに接続して読み続ける。切れたら 3 秒後に再接続。"""
    while True:
        writer = None
        try:
            reader, writer = await asyncio.open_connection(SBS_HOST, SBS_PORT)
            log.info("SBS に接続しました %s:%d", SBS_HOST, SBS_PORT)
            while line := await reader.readline():
                handle_sbs_line(line.decode("ascii", "ignore").strip())
            log.warning("SBS の接続が閉じられました")
        except OSError as e:
            log.warning("SBS に接続できません: %s", e)
        finally:
            if writer:
                writer.close()
        await asyncio.sleep(3)


# ---------------------------------------------------------------- 配信

def purge_stale(now: float) -> None:
    for icao in [k for k, a in aircraft.items() if now - a["_seen"] > STALE_SEC]:
        del aircraft[icao]


def snapshot() -> str:
    now = time.time()
    purge_stale(now)
    items = []
    for a in aircraft.values():
        d = {k: v for k, v in a.items() if not k.startswith("_")}
        d["age"] = round(now - a["_seen"], 1)               # 最終受信からの秒数
        if "_pos_seen" in a:
            d["pos_age"] = round(now - a["_pos_seen"], 1)   # 位置更新からの秒数
        items.append(d)
    return json.dumps(
        {"type": "aircraft", "now": round(now, 1), "count": len(items), "aircraft": items},
        ensure_ascii=False,
        separators=(",", ":"),
    )


async def _send(ws, msg: str) -> None:
    try:
        await asyncio.wait_for(ws.send(msg), timeout=5)
    except Exception:
        pass  # 切断済みや遅いクライアントは今回は飛ばす(後始末は handler 側)


async def broadcaster() -> None:
    while True:
        await asyncio.sleep(PUSH_INTERVAL)
        if not clients:
            purge_stale(time.time())
            continue
        msg = snapshot()
        await asyncio.gather(*(_send(ws, msg) for ws in list(clients)))


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


def token_ok(ws) -> bool:
    query = parse_qs(urlparse(_request_path(ws)).query)
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
    if not token_ok(ws):
        log.warning("認証失敗: %s", peer)
        await ws.close(code=1008, reason="unauthorized")
        return
    if len(clients) >= MAX_CLIENTS:
        await ws.close(code=1013, reason="too many clients")
        return

    clients.add(ws)
    log.info("接続: %s (計 %d)", peer, len(clients))
    try:
        await ws.send(snapshot())        # 接続直後に 1 回送る
        async for _ in ws:               # クライアントからのメッセージは今は無視
            pass
    except websockets.ConnectionClosed:
        pass
    finally:
        clients.discard(ws)
        log.info("切断: %s (計 %d)", peer, len(clients))


async def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    if not TOKENS:
        log.error("KAWAEYES_TOKENS が設定されていません")
        raise SystemExit(1)
    log.info("websockets %s / 待ち受け %s:%d", websockets.__version__, WS_HOST, WS_PORT)
    async with websockets.serve(
        handler, WS_HOST, WS_PORT,
        max_size=4096,       # クライアントから受け取る最大サイズ
        ping_interval=20,    # Cloudflare の無通信切断対策
        ping_timeout=20,
    ):
        await asyncio.gather(sbs_reader(), broadcaster())


if __name__ == "__main__":
    asyncio.run(main())
