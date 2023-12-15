#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    sudo "$0" "$@"
    exit 0
fi

echo "---- Installing gamescope-session..."

if [ -f /etc/os-release ]; then
    if [ ! -f /etc/os-release.bak_gcs ]; then
        cp /etc/os-release /etc/os-release.bak_gcs
    fi
fi
if [ -f /usr/bin/steam-runtime ]; then
    if [ ! -f /usr/bin/steam-runtime.bak_gcs ]; then
        cp /usr/bin/steam-runtime /usr/bin/steam-runtime.bak_gcs
    fi
fi
if [ -f /usr/bin/steam-jupiter ]; then
    if [ ! -f /usr/bin/steam-jupiter.bak_gcs ]; then
        cp /usr/bin/steam-jupiter /usr/bin/steam-jupiter.bak_gcs
    fi
fi
cp etc / -rf
cp usr / -rf
cp home / -rf

echo "---- Install complete..."

cd GameAssistService
./install.sh
cd ..

echo "---- Install complete..."
