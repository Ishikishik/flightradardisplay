# ライブラリ自動で入れる
# トークンを発行する
set -euo pipefail

SERVICE=kawaeyes-server
DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
RUN_USER=$(stat -c %U "$DIR")

if [ "$(id -u)" -ne 0 ]; then
  echo "sudo で実行してください: sudo bash $0" >&2
  exit 1
fi

# ---- 1. apt パッケージ
missing=()
while IFS= read -r line; do
  pkg=$(echo "${line%%#*}" | xargs)
  [ -z "$pkg" ] && continue
  dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed" || missing+=("$pkg")
done < "$DIR/requirements-apt.txt"

if [ ${#missing[@]} -gt 0 ]; then
  echo "[setup] パッケージをインストール: ${missing[*]}"
  apt-get update -qq
  DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "${missing[@]}"
else
  echo "[setup] パッケージ: 追加なし"
fi

# ---- 2. .env(トークン)
ENV_FILE="$DIR/.env"
if ! grep -qs '^KAWAEYES_TOKENS=.' "$ENV_FILE"; then
  token=$(python3 -c 'import secrets; print(secrets.token_urlsafe(24))')
  if [ -s "$ENV_FILE" ] && [ -n "$(tail -c1 "$ENV_FILE")" ]; then echo >> "$ENV_FILE"; fi
  echo "KAWAEYES_TOKENS=$token" >> "$ENV_FILE"
  echo "[setup] トークンを生成して .env に保存しました"
  if [ -t 1 ]; then
    echo "        トークン: $token"
  else
    echo "        (Pi で sudo cat $ENV_FILE を実行して確認)"   # CI のログには出さない
  fi
else
  echo "[setup] .env: 既存のトークンを使用"
fi
chown "$RUN_USER": "$ENV_FILE"
chmod 600 "$ENV_FILE"

# ---- 3. systemd ユニット
unit_tmp=$(mktemp)
sed -e "s|@DIR@|$DIR|g" -e "s|@USER@|$RUN_USER|g" "$DIR/$SERVICE.service" > "$unit_tmp"
dest="/etc/systemd/system/$SERVICE.service"
if ! cmp -s "$unit_tmp" "$dest"; then
  install -m 644 "$unit_tmp" "$dest"
  systemctl daemon-reload
  echo "[setup] systemd ユニットを更新しました"
fi
rm -f "$unit_tmp"
systemctl enable --quiet "$SERVICE"

# ---- 4. 構文チェック → 再起動
python3 -c 'import ast, sys; ast.parse(open(sys.argv[1]).read(), sys.argv[1])' "$DIR/adsb_ws_server.py"
systemctl restart "$SERVICE"
sleep 3
if systemctl is-active --quiet "$SERVICE"; then
  echo "[setup] $SERVICE は稼働中です"
else
  echo "[setup] $SERVICE の起動に失敗しました" >&2
  journalctl -u "$SERVICE" -n 20 --no-pager >&2
  exit 1
fi
