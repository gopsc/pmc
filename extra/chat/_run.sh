#!/bin/bash
cd "$(dirname "$0")" || exit
bash ./_set.sh 1>/dev/null
source ./.env/bin/activate
exec python server.py --type=server --port=8003 --addr=0.0.0.0
