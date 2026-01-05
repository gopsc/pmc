#!/bin/bash
cd "$(dirname "$0")" || exit
bash ./_setup.sh 1>/dev/null
source ./.env/bin/activate
exec python init.py
