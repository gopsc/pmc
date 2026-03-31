#!/bin/bash
cd "$(dirname "$0")" || exit
#source ./.env/bin/activate
source /cangjie/envsetup.sh
exec cjpm run
