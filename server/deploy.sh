set -e
. ./.env
: "${PI_HOST:?ルートの .env に PI_HOST を設定してください}"
cd "$(dirname "$0")/.."

rsync -az --delete --exclude '.env' --exclude '.venv' --exclude '__pycache__' \
  server/ "$PI_HOST:$PI_DIR/"
ssh "$PI_HOST" "sudo systemctl restart $SERVICE"