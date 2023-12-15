#!/bin/bash

set -eu

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

if [ $# -ge 1 ]; then

	VENDOR=$(cat /proc/cpuinfo | grep 'vendor' | uniq)
	AMD='AMD'
	INTEL='Intel'

	CONFIG_DIR=/home/deck/.config/GameAssist
	if [ ! -d $CONFIG_DIR ]; then
		mkdir $CONFIG_DIR
	fi

	echo $1 > $CONFIG_DIR/gpu.txt

	REL_DIR=$(dirname $0)
	if [[ "$VENDOR" == *"$AMD"* ]]; then
		$REL_DIR/cmdGPU.sh power_dpm_force_performance_level manual
		$REL_DIR/cmdGPU.sh pp_od_clk_voltage s 0 $1
		$REL_DIR/cmdGPU.sh pp_od_clk_voltage s 1 $1
		$REL_DIR/cmdGPU.sh pp_od_clk_voltage c
	elif [[ "$VENDOR" == *"$INTEL"* ]]; then
		:
	fi
	
fi
