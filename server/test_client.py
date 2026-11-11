#!/usr/bin/env python3
"""配信を受信してバイナリを解読し、表で表示する確認用クライアント(Mac などで使う)。

  python3 server/test_client.py              # リポジトリ直下の .env の KAWAEYES_URL を使う
  python3 server/test_client.py "wss://..."  # URL を直接指定
  python3 server/test_client.py --json       # JSON 形式で受信して表示
"""
import asyncio
import os
import struct
import sys
from pathlib import Path

import websockets

HEADER = struct.Struct("<BBH")
RECORD = struct.Struct("<3sBiihBB8s")


def decode(msg: bytes) -> None:
    version, mtype, count = HEADER.unpack_from(msg, 0)
    print(f"--- v{version} type={mtype} {count} 機 ({len(msg)} B)")
    for i in range(count):
        icao_b, flags, lat, lon, alt, cat, track, cs = RECORD.unpack_from(msg, HEADER.size + i * RECORD.size)
        icao = int.from_bytes(icao_b, "little")
        pos = f"{lat / 1e5:9.5f},{lon / 1e5:10.5f}" if flags & 1 else " " * 20
        altitude = "ground" if flags & 8 else (f"{alt * 25:6d}ft" if flags & 2 else "")
        trk = f"{track * 360 / 256:5.1f}°" if flags & 16 else ""
        category = f"{cat:02X}" if cat else "--"
        callsign = cs.decode("ascii", "replace").strip() if flags & 4 else ""
        print(f"{icao:06X} {callsign:8} {pos} {altitude:>8} {trk:>6} {category}")


async def main(url: str) -> None:
    async with websockets.connect(url) as ws:
        async for msg in ws:
            if isinstance(msg, bytes):
                decode(msg)
            else:
                print(msg)


def find_url(args: list[str]) -> str:
    urls = [a for a in args if a.startswith("ws")]
    url = urls[0] if urls else os.environ.get("KAWAEYES_URL", "")
    if not url:
        env = Path(__file__).resolve().parent.parent / ".env"   # リポジトリ直下(git 管理外)
        if env.exists():
            for line in env.read_text().splitlines():
                if line.startswith("KAWAEYES_URL="):
                    url = line.split("=", 1)[1].strip().strip('"')
    if not url:
        sys.exit("URL がありません。引数か、リポジトリ直下の .env に KAWAEYES_URL=... を書いてください")
    if "--json" in args:
        url += "&format=json"
    return url


if __name__ == "__main__":
    asyncio.run(main(find_url(sys.argv[1:])))
