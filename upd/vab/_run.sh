#!/bin/bash
cd "$(dirname "$0")" || exit
#bash ./_setup.sh
source ./.env/bin/activate
exec python3 src/main.py
