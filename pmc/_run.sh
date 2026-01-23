#!/bin/bash
cd "$(dirname "$0")" || exit
./pmcd --pmc --sys=.. --addr=0.0.0.0 --port=8012 --key=./public_key.pem  --mybot
