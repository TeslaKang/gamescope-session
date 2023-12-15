#!/bin/bash

set -eu

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

CONFIG_DIR=/home/deck/.config/GameAssist
FILE=$CONFIG_DIR/cpu.txt
if [ -f $FILE ]; then
    rm $FILE
fi
