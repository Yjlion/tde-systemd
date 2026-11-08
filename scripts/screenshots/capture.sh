#!/bin/bash
# capture.sh <output.png> <command...>
# Runs a command in the demo X session and captures the screen, trimmed to
# the window. WAIT sets the seconds to wait before the capture.
out=$1; shift
pkill -u demo -f '^(/opt/trinity/bin/)?(tde-systemd-manager|tde-journal-viewer|kcontrol|tdecmshell|khelpcenter)( |$)' 2>/dev/null; sleep 1
su demo -c ". /tmp/demo-env; $* >/tmp/app.log 2>&1 &"
sleep "${WAIT:-10}"
DISPLAY=:1 import -window root /tmp/raw.png
convert /tmp/raw.png -bordercolor black -border 1 -trim +repage "$out"
