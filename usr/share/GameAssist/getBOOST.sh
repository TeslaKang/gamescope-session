#!/bin/bash

set -eu

if [[ "$(cat /sys/devices/system/cpu/cpufreq/boost)" == "1" ]]; then
	echo "1"
else	
	echo "0"
fi
