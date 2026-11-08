#!/bin/bash
# start an X session for user demo inside the test container
pkill Xvfb || true; sleep 1
Xvfb :1 -screen 0 1280x860x24 -nolisten tcp >/tmp/xvfb.log 2>&1 &
sleep 2
U=$(id -u demo)
cat > /tmp/demo-env <<EOT
export DISPLAY=:1
export HOME=/home/demo
export USER=demo
export XDG_RUNTIME_DIR=/run/user/$U
export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$U/bus
export PATH=/opt/trinity/bin:/usr/sbin:/usr/bin:/sbin:/bin
export LANG=en_US.UTF-8
export XDG_CONFIG_DIRS=/opt/trinity/etc/xdg:/etc/xdg
export XDG_DATA_DIRS=/opt/trinity/share:/usr/share
export TDEDIRS=/opt/trinity
EOT
chmod 644 /tmp/demo-env
su demo -c '. /tmp/demo-env; dcopserver --nosid >/tmp/dcop.log 2>&1; tdeinit >/tmp/tdeinit.log 2>&1 & sleep 3; twin >/tmp/twin.log 2>&1 & sleep 2; polkit-agent-tde >/tmp/agent.log 2>&1 &'
sleep 2
echo started
