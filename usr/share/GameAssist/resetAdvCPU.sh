#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

CONFIG_DIR=/home/deck/.config/GameAssist
FILE=$CONFIG_DIR/smt.txt
if [ -f $FILE ]; then
    rm $FILE
fi

FILE=$CONFIG_DIR/boost.txt
if [ -f $FILE ]; then
    rm $FILE
fi

FILE=$CONFIG_DIR/policy.txt
if [ -f $FILE ]; then
    rm $FILE
fi

echo "on" > /sys/devices/system/cpu/smt/control
echo "1" > /sys/devices/system/cpu/cpufreq/boost
cpupower frequency-set -g performance

min_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq)
max_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq)
cpupower frequency-set --min $min_freq --max $max_freq
