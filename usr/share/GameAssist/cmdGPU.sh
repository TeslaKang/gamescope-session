#!/bin/bash

set -eu

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

if [ $# -ge 2 ]; then

    if [ -e /sys/class/drm/card0/device/$1 ]; then
        echo ${@:2} > /sys/class/drm/card0/device/$1
    elif [ -e /sys/class/drm/card1/device/$1 ]; then
        echo ${@:2} > /sys/class/drm/card1/device/$1
    elif [ -e /sys/class/drm/card2/device/$1 ]; then
        echo ${@:2} > /sys/class/drm/card2/device/$1
    fi

fi

