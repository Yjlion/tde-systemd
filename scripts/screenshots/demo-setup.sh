#!/bin/bash
# demo data for screenshots (runs as root in the test container)
mkdir -p /etc/polkit-1/rules.d
cat > /etc/polkit-1/rules.d/49-tde-systemd-demo.rules <<'EOT'
polkit.addRule(function(action, subject) {
    if (action.id.indexOf("org.trinitydesktop.tde-systemd.") == 0 && subject.user == "demo")
        return polkit.Result.YES;
});
EOT
cat > /etc/systemd/system/backup-demo.service <<'EOT'
[Unit]
Description=Nightly backup of /home (demo)
[Service]
Type=oneshot
ExecStart=/bin/sh -c 'echo "Backup started"; sleep 1; echo "Backup of /home finished: 1532 files"'
EOT
cat > /etc/systemd/system/backup-demo.timer <<'EOT'
[Unit]
Description=Run the nightly backup (demo)
[Timer]
OnCalendar=*-*-* 02:30:00
Persistent=true
[Install]
WantedBy=timers.target
EOT
cat > /etc/systemd/system/webapp-demo.service <<'EOT'
[Unit]
Description=Example web application (demo)
[Service]
ExecStart=/bin/sh -c 'echo "Listening on port 8080"; while true; do sleep 60; echo "heartbeat"; done'
[Install]
WantedBy=multi-user.target
EOT
cat > /etc/systemd/system/broken-demo.service <<'EOT'
[Unit]
Description=Service with a broken configuration (demo)
[Service]
ExecStart=/bin/sh -c 'echo "Cannot open /etc/broken-demo.conf: No such file or directory" >&2; exit 1'
EOT
mkdir -p /home/demo/.config/systemd/user
cat > /home/demo/.config/systemd/user/sync-notes.service <<'EOT'
[Unit]
Description=Synchronize notes folder (user demo)
[Service]
ExecStart=/bin/sh -c 'echo "Watching ~/Notes for changes"; while true; do sleep 300; done'
[Install]
WantedBy=default.target
EOT
chown -R demo:demo /home/demo/.config
systemctl daemon-reload
systemctl enable --now webapp-demo.service backup-demo.timer >/dev/null 2>&1
systemctl start backup-demo.service broken-demo.service >/dev/null 2>&1
systemctl restart polkit
su demo -c '. /tmp/demo-env; systemctl --user daemon-reload; systemctl --user enable --now sync-notes.service' >/dev/null 2>&1
logger -p daemon.err -t webapp "Connection to database failed: timeout after 30s"
logger -p daemon.warning -t webapp "Slow request: GET /api/report took 4.2s"
logger -p auth.notice -t sshd "Accepted publickey for demo from 192.168.1.20 port 51234"
logger -p user.info -t cron "(demo) CMD (run-parts /etc/cron.hourly)"
logger -p kern.crit -t kernel-demo "CPU0: Core temperature above threshold, cpu clock throttled"
su demo -c 'logger -t tde-session "Session started for demo"'
