#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    sudo "$0" "$@"
    exit 0
fi

echo "---- Removing gamescope-session..."

if [ -f /etc/os-release.bak_gcs ]; then
    rm -v -f /etc/os-release
    mv /etc/os-release.bak_gcs /etc/os-release
fi
if [ -f /usr/bin/steam-runtime.bak_gcs ]; then
    rm -v -f /usr/bin/steam-runtime
    mv /usr/bin/steam-runtime.bak_gcs /usr/bin/steam-runtime
fi
if [ -f /usr/bin/steam-jupiter.bak_gcs ]; then
    rm -v -f /usr/bin/steam-jupiter
    mv /usr/bin/steam-jupiter.bak_gcs /usr/bin/steam-jupiter
fi
rm -v -f /etc/sddm.conf.d/steamos.conf
rm -v -f /etc/sddm.conf.d/zz-steamos-autologin.conf

rm -v -f ~/Desktop/steamos-gamemode.desktop

rm -v -f /usr/bin/export-gpu
rm -v -f /usr/bin/gamescope-session-plus
rm -v -f /usr/bin/gamescope-wayland-teardown-workaround
rm -v -f /usr/bin/jupiter-biosupdate
rm -v -f /usr/bin/steam-http-loader
rm -v -f /usr/bin/steamos-readonly
rm -v -f /usr/bin/steamos-reboot
rm -v -f /usr/bin/steamos-select-branch
rm -v -f /usr/bin/steamos-session-select
rm -v -f /usr/bin/steamos-update
rm -v -r /usr/bin/steamos-polkit-helpers

rm -v -f /usr/lib/os-branch-select
rm -v -f /usr/lib/os-session-desktop
rm -v -f /usr/lib/os-session-gamescope
rm -v -f /usr/lib/os-session-select
rm -v -f /usr/lib/systemd/user/gamescope-session-plus@.service

rm -v -f /usr/share/applications/gamescope-mimeapps.list
rm -v -f /usr/share/applications/steam_http_loader.desktop

rm -v -r /usr/share/gamescope-session-plus

rm -v -f /usr/share/icons/hicolor/scalable/apps/distributor-logo-steamdeck.svg
rm -v -f /usr/share/icons/hicolor/scalable/apps/steamdeck-gaming-return.svg

rm -v -f /usr/share/pixmaps/steamos.png

rm -v -f /usr/share/polkit-1/actions/org.gameassist.update.policy
rm -v -f /usr/share/polkit-1/actions/org.gamescope.update.policy
rm -v -f /usr/share/polkit-1/actions/org.valve.steamos.policy

rm -v -f /usr/share/wayland-sessions/gamescope-session-steam.desktop
rm -v -f /usr/share/wayland-sessions/gamescope-session.desktop

cd GameAssistService
./remove.sh
cd ..

echo "---- Remove complete."
