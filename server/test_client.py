#!/usr/bin/env python3
"""配信を受信してバイナリを解読し、表で表示する確認用クライアント(Mac などで使う)。

  python3 test_client.py "wss://kawaradar.otterstb.com/?token=xxxx"
"""
import asyncio
import struct
import sys

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


if __name__ == "__main__":
    asyncio.run(main(sys.argv[1]))
