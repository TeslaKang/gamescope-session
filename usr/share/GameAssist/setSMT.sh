#!/bin/bash

set -eu

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

if [ $# -ge 1 ]; then

	CONFIG_DIR=/home/deck/.config/GameAssist
	if [ ! -d $CONFIG_DIR ]; then
		mkdir $CONFIG_DIR
	fi

	echo $1 > $CONFIG_DIR/smt.txt

	echo $1 > /sys/devices/system/cpu/smt/control
	
fi
