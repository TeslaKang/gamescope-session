#!/bin/bash

set -eu

if [[ "$(cat /sys/devices/system/cpu/smt/control)" == "on" ]]; then
	echo "1"
else	
	echo "0"
fi
