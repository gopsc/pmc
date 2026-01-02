#!/bin/bash
cd "$(dirname "$0")" || exit
python3 -m venv .env
source ./.env/bin/activate
source ./_src.sh
#python -m pip install --upgrade pip
pip install -r requirements.txt
