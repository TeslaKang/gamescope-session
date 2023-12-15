#!/bin/bash

set -eu

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

CONFIG_DIR=/home/deck/.config/GameAssist
FILE=$CONFIG_DIR/tdp.txt
if [ -f $FILE ]; then
    rm $FILE

    REL_DIR=$(dirname $0)
    $REL_DIR/setTDP.sh 15 --no-config
fi
