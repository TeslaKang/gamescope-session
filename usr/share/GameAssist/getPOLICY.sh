#!/bin/bash

set -eu

policy=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
if [[ "$policy" == "performance" ]]; then
	echo "2"
elif [[ "$policy" == "ondemand" ]]; then
	echo "1"
elif [[ "$policy" == "powersave" ]]; then
	echo "0"
else
	echo "0"
fi
