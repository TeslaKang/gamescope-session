#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

REL_DIR=$(dirname $0)
$REL_DIR/GameAssist on

CONFIG_DIR=/home/deck/.config/GameAssist
FILE=$CONFIG_DIR/tdp.txt
if [ -f $FILE ]; then
    TDP=$(cat $FILE)
    c=$(echo "$TDP < 13" | bc)
    if [ $c == '1' ]; then
        $REL_DIR/setTDP.sh 13 --no-config
    fi
fi
