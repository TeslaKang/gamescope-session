#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

REL_DIR=$(dirname $0)
if [ $# -ge 1 ]; then
    $REL_DIR/GameAssist $1
else
    $REL_DIR/GameAssist
fi

$REL_DIR/GameAssist off

CONFIG_DIR=/home/deck/.config/GameAssist
FILE=$CONFIG_DIR/tdp.txt
if [ -f $FILE ]; then
    TDP=$(cat $FILE)
    c=$(echo "$TDP < 15" | bc)
    if [ $c == '1' ]; then
        $REL_DIR/setTDP.sh 15 --no-config
    fi
fi
