#!bin/bash
cd "$(dirname "$0")" || exit
exec ./lci --lci --rotate=0 --fontsize=15
