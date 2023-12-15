#!/bin/bash
set -e
echo "Installing Game Assist service..."

cp ga_suspend_after.service /usr/lib/systemd/system/
cp ga_suspend_before.service /usr/lib/systemd/system/
cp ga_hibernate_after.service /usr/lib/systemd/system/
cp ga_hibernate_before.service /usr/lib/systemd/system/
cp ga_start.service /usr/lib/systemd/system/

systemctl enable ga_suspend_after && systemctl start ga_suspend_after
systemctl enable ga_suspend_before && systemctl start ga_suspend_before
systemctl enable ga_hibernate_after && systemctl start ga_hibernate_after
systemctl enable ga_hibernate_before && systemctl start ga_hibernate_before
systemctl enable ga_start && systemctl start ga_start
