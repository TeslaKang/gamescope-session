#!/bin/bash

set -eu

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

if [ $# -ge 1 ]; then

	VENDOR=$(cat /proc/cpuinfo | grep 'vendor' | uniq)
	AMD='AMD'
	INTEL='Intel'

	WRITE_CONFIG='1'
	if [ $# -ge 2 ]; then
		if [[ $2 == "--no-config" ]]; then
			WRITE_CONFIG='0'
		fi
	fi

	if [[ $WRITE_CONFIG == "1" ]]; then
		CONFIG_DIR=/home/deck/.config/GameAssist
		if [ ! -d $CONFIG_DIR ]; then
			mkdir $CONFIG_DIR
		fi

		echo $1 > $CONFIG_DIR/tdp.txt
	fi		

	REL_DIR=$(dirname $0)
	if [[ "$VENDOR" == *"$AMD"* ]]; then

		scale=1000
#		ftdp=$(echo "$1*$scale" | bc)
		ftdp=$(echo $1 $scale | awk '{printf "%4.3f\n",$1*$2}')
		tdp=${ftdp%.*}
		$REL_DIR/ryzenadj-run.sh --stapm-limit=$tdp --fast-limit=$tdp --slow-limit=$tdp

	elif [[ "$VENDOR" == *"$INTEL"* ]]; then

		scale=1000000
#		ftdp=$(echo "$1*$scale" | bc)
		ftdp=$(echo $1 $scale | awk '{printf "%4.3f\n",$1*$2}')
		tdp=${ftdp%.*}
		echo $tdp > /sys/class/powercap/intel-rapl/intel-rapl:0/constraint_0_power_limit_uw
		echo $tdp > /sys/class/powercap/intel-rapl/intel-rapl:0/constraint_1_power_limit_uw

	fi
fi
