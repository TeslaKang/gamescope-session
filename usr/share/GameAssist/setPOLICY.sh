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

	echo $1 > $CONFIG_DIR/policy.txt

	if [[ "$1" == "0" ]]; then
		cpupower frequency-set -g powersave
	elif [[ "$1" == "1" ]]; then
		cpupower frequency-set -g ondemand
	elif [[ "$1" == "2" ]]; then
		cpupower frequency-set -g performance
	fi

fi
