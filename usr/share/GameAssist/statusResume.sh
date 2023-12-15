#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

REL_DIR=$(dirname $0)

CONFIG_DIR=/home/deck/.config/GameAssist
FILE=$CONFIG_DIR/policy.txt
if [ -f $FILE ]; then
    SMT=$(cat $FILE)
    if [ $SMT == '0' ]; then
        $REL_DIR/setPOLICY.sh $SMT
    elif [ $SMT == '1' ]; then
        $REL_DIR/setPOLICY.sh $SMT
    elif [ $SMT == '2' ]; then
        $REL_DIR/setPOLICY.sh $SMT
    fi
fi

FILE=$CONFIG_DIR/cpu.txt
if [ -f $FILE ]; then
    CPU=$(cat $FILE)
    c=$(echo "$CPU > 200" | bc)
    if [ $c == '1' ]; then
        $REL_DIR/setCPU.sh $CPU
    fi
fi

FILE=$CONFIG_DIR/gpu.txt
if [ -f $FILE ]; then
    GPU=$(cat $FILE)
    c=$(echo "$GPU > 900" | bc)
    if [ $c == '1' ]; then
        $REL_DIR/setGPU.sh $GPU
    fi
fi

FILE=$CONFIG_DIR/smt.txt
if [ -f $FILE ]; then
    SMT=$(cat $FILE)
    if [ $SMT == 'off' ]; then
        $REL_DIR/setSMT.sh $SMT
    elif [ $SMT == 'on' ]; then
        $REL_DIR/setSMT.sh $SMT
    fi
fi

FILE=$CONFIG_DIR/boost.txt
if [ -f $FILE ]; then
    BOOST=$(cat $FILE)
    if [ $SMT == '0' ]; then
        $REL_DIR/setBOOST.sh $SMT
    elif [ $SMT == '1' ]; then
        $REL_DIR/setBOOST.sh $SMT
    fi
fi

FILE=$CONFIG_DIR/tdp.txt
if [ -f $FILE ]; then
    TDP=$(cat $FILE)
    c=$(echo "$TDP > 2" | bc)
    if [ $c == '1' ]; then
        $REL_DIR/setTDP.sh $TDP
    fi
fi

$REL_DIR/GameAssist off
