#!/bin/bash
cd "$(dirname "$0")" || exit
bash ./_setup.sh
source ./.env/bin/activate
exec python hubs.py
