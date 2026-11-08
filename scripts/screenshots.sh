#!/bin/bash
# Create the screenshots in screenshots/ and doc/en/.
#
# Builds a systemd-booted Debian container with TDE, compiles and installs
# tde-systemd in it, creates some demo units and journal messages, and
# captures the applications running under Xvfb. Needs Docker and a host
# with cgroup v2 (the container runs privileged).

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DIST=${DIST:-bookworm}
IMAGE=tde-systemd-screenshots:$DIST
NAME=tde-systemd-screenshots
OUT=$ROOT/screenshots

docker build -t "$IMAGE" --build-arg DIST="$DIST" "$ROOT/scripts/screenshots"
docker rm -f "$NAME" >/dev/null 2>&1 || true
docker run -d --name "$NAME" --privileged --cgroupns=host \
    -v /sys/fs/cgroup:/sys/fs/cgroup:rw -v "$ROOT":/src:ro "$IMAGE" >/dev/null
trap 'docker rm -f "$NAME" >/dev/null 2>&1 || true' EXIT
sleep 5

run() { docker exec "$NAME" bash -c "$*"; }

echo "==> building tde-systemd"
run 'mkdir -p /b && cd /b && cmake /src -DCMAKE_INSTALL_PREFIX=/opt/trinity >/dev/null && make -j"$(nproc)" >/dev/null && make install >/dev/null'

echo "==> preparing the demo session"
docker cp "$ROOT/scripts/screenshots/." "$NAME":/shots-tools
run 'id demo >/dev/null 2>&1 || useradd -m -s /bin/bash -G systemd-journal demo; loginctl enable-linger demo; sleep 2'
run 'bash /shots-tools/session.sh && bash /shots-tools/demo-setup.sh'
run 'su demo -c ". /tmp/demo-env; tdebuildsycoca --noincremental" >/dev/null 2>&1'
H=/opt/trinity/lib/trinity/libexec/tde-systemd-helper
run "su demo -c '. /tmp/demo-env; pkexec $H locale set-locale LANG=en_US.UTF-8 LC_TIME=de_DE.UTF-8; pkexec $H hostname set-pretty \"Demo Workstation\"; pkexec $H hostname set-chassis desktop; pkexec $H hostname set-location \"Office 2.14\"; pkexec $H timedate set-timezone America/New_York'"

echo "==> capturing"
run 'mkdir -p /shots && rm -f /shots/* && chmod 1777 /shots'
cap() { run "WAIT=${WAIT:-10} bash /shots-tools/capture.sh /shots/$1.png $2"; }
cap manager-overview  "tde-systemd-manager --page overview"
cap manager-services  "tde-systemd-manager --unit webapp-demo.service"
cap manager-timers    "tde-systemd-manager --page timers"
cap manager-datetime  "tde-systemd-manager --page datetime"
cap manager-hostname  "tde-systemd-manager --page hostname"
WAIT=14 cap manager-locale "tde-systemd-manager --page locale"
cap journal-viewer    "tde-journal-viewer --priority 4"
cap journal-maintenance "tde-journal-viewer --maintenance --priority 5"
WAIT=14 cap handbook  "khelpcenter help:/tde-systemd/manager-services.html"
run 'su demo -c "bash /shots-tools/kcontrol.sh /shots"'

docker cp "$NAME":/shots/. "$OUT/"
for f in "$ROOT"/doc/en/*.png; do cp "$OUT/$(basename "$f")" "$f"; done
echo "==> screenshots written to $OUT"
