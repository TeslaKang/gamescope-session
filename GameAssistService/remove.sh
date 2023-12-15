#!/bin/bash
set -e

echo "---- Stop Game Assist service..."
systemctl stop ga_suspend_after && systemctl disable ga_suspend_after
systemctl stop ga_suspend_before && systemctl disable ga_suspend_before
systemctl stop ga_hibernate_after && systemctl disable ga_hibernate_after
systemctl stop ga_hibernate_before && systemctl disable ga_hibernate_before
systemctl stop ga_start && systemctl disable ga_start

echo "---- Removing Game Assist service..."
rm -v -f /usr/lib/systemd/system/ga_suspend_after.service
rm -v -f /usr/lib/systemd/system/ga_suspend_before.service
rm -v -f /usr/lib/systemd/system/ga_hibernate_after.service
rm -v -f /usr/lib/systemd/system/ga_hibernate_before.service
rm -v -f /usr/lib/systemd/system/ga_start.service
