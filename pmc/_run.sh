#!/bin/bash
cd "$(dirname "$0")" || exit
./pmcd --sys=.. --addr=127.0.0.1 --port=8012 --key=./public_key.pem
