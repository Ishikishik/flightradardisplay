"""デプロイ確認用のテストスクリプト。

GitHub Actions から Pi に配置されたあと、Pi 上で実行されて
ホスト名・Python のバージョン・現在時刻を表示する。
"""
import datetime
import platform
import socket

VERSION = "0.1.0"


def main() -> None:
    now = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    print(f"kawaeyes server hello v{VERSION}")
    print(f"  host   : {socket.gethostname()}")
    print(f"  python : {platform.python_version()}")
    print(f"  time   : {now}")


if __name__ == "__main__":
    main()
