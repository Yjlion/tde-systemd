#!/bin/bash
# kcontrol.sh <outdir>: Control Center with the System Administration
# category and both modules (runs as user demo)
. /tmp/demo-env
out=$1
pkill -u demo -f '^(/opt/trinity/bin/)?kcontrol( |$)'; sleep 1
kcontrol >/tmp/app.log 2>&1 &
sleep 10
W=$(xdotool search --name "Trinity Control Center" | head -1)
xdotool windowmove "$W" 0 0; xdotool windowsize "$W" 1240 820; sleep 2
shot() { import -window root /tmp/kc-raw.png; convert /tmp/kc-raw.png -bordercolor black -border 1 -trim +repage "$1"; }
# tree rows: "System Administration" and its entries (positions for the default style)
xdotool mousemove 128 249 click 1; sleep 3; shot "$out/kcontrol-category.png"
xdotool mousemove 130 369 click 1; sleep 8; shot "$out/kcontrol-systemd.png"
xdotool mousemove 120 389 click 1; sleep 8; shot "$out/kcontrol-journal.png"
pkill -u demo -f '^(/opt/trinity/bin/)?kcontrol( |$)' || true
