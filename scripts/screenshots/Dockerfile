# systemd-booted Debian with TDE, used to take the screenshots
ARG DIST=bookworm
FROM debian:${DIST}
ARG DIST
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates wget && \
    wget -q http://mirror.ppa.trinitydesktop.org/trinity/deb/trinity-keyring.deb -O /tmp/k.deb && \
    apt-get install -y /tmp/k.deb && \
    echo "deb http://mirror.ppa.trinitydesktop.org/trinity/deb/trinity-r14.1.x ${DIST} main deps" \
        > /etc/apt/sources.list.d/trinity.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential cmake pkg-config tde-cmake tdelibs14-trinity-dev \
        systemd-sysv dbus dbus-user-session polkitd pkexec systemd-timesyncd \
        locales keyboard-configuration console-setup kbd cron \
        xvfb x11-xserver-utils x11-utils imagemagick xdotool fonts-dejavu-core \
        twin-trinity kcontrol-trinity khelpcenter-trinity polkit-agent-tde tdelibs-data-trinity && \
    apt-get clean && \
    sed -i "s/^# *en_US.UTF-8/en_US.UTF-8/; s/^# *de_DE.UTF-8/de_DE.UTF-8/" /etc/locale.gen && locale-gen
CMD ["/sbin/init"]
