#!/bin/sh
# Build the tde-systemd Debian package inside a Debian container.
#
#   scripts/build-deb.sh [distribution] [output directory]
#
# Must run as root in a clean Debian system (e.g. the debian:bookworm or
# debian:trixie container image). Adds the TDE R14.1 repository, installs
# the build dependencies and builds the binary packages. The package
# version gets a "~<distribution>" suffix so that packages for different
# distributions can be published side by side.

set -eu

SRC=$(cd "$(dirname "$0")/.." && pwd)
DIST=${1:-$(. /etc/os-release && echo "$VERSION_CODENAME")}
OUT=$(mkdir -p "${2:-$SRC/dist}" && cd "${2:-$SRC/dist}" && pwd)
TDE_MIRROR=${TDE_MIRROR:-http://mirror.ppa.trinitydesktop.org/trinity/deb/trinity-r14.1.x}

export DEBIAN_FRONTEND=noninteractive

echo "==> Adding the TDE repository for $DIST"
apt-get update
apt-get install -y --no-install-recommends ca-certificates wget
wget -q "${TDE_MIRROR%/trinity-r14.1.x}/trinity-keyring.deb" -O /tmp/trinity-keyring.deb
apt-get install -y /tmp/trinity-keyring.deb
echo "deb $TDE_MIRROR $DIST main deps" > /etc/apt/sources.list.d/trinity.list
apt-get update

echo "==> Installing build dependencies"
apt-get install -y --no-install-recommends build-essential devscripts equivs lintian
BUILD=$(mktemp -d)
cp -a "$SRC" "$BUILD/tde-systemd"
cd "$BUILD/tde-systemd"
rm -rf dist build obj-*
mk-build-deps --install --remove \
    --tool "apt-get -y --no-install-recommends" debian/control

echo "==> Building"
sed -i "1s/(\([^)~]*\))/(\1~$DIST)/" debian/changelog
dpkg-buildpackage -b -us -uc

echo "==> lintian"
lintian --info --display-info ../tde-systemd_*.changes || true

mv ../*.deb "$OUT/"
echo "==> Packages in $OUT:"
ls -l "$OUT"
