/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "datetimepage.h"
#include "sdprocess.h"

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqdatetime.h>
#include <tqdatetimeedit.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqmap.h>
#include <tqtimer.h>

#include <kiconloader.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <kpushbutton.h>

DateTimePage::DateTimePage(TQWidget *parent, const char *name)
    : SdPage(parent, name), m_curNtp(false), m_curLocalRtc(false), m_canNtp(false), m_timeEdited(false)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 8);

    TQGroupBox *now = new TQGroupBox(i18n("Current Time"), this);
    now->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *ng = new TQGridLayout(now->layout(), 3, 3, 6);
    TQLabel *icon = new TQLabel(now);
    icon->setPixmap(DesktopIcon("date", 64));
    ng->addMultiCellWidget(icon, 0, 2, 0, 0);
    m_clock = new TQLabel(now);
    TQFont f = m_clock->font();
    f.setPointSize(f.pointSize() * 2);
    f.setBold(true);
    m_clock->setFont(f);
    ng->addWidget(m_clock, 0, 1);
    m_utc = new TQLabel(now);
    ng->addWidget(m_utc, 1, 1);
    m_sync = new TQLabel(now);
    ng->addWidget(m_sync, 2, 1);
    ng->setColStretch(1, 1);
    top->addWidget(now);

    TQGroupBox *set = new TQGroupBox(i18n("Settings"), this);
    set->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *sg = new TQGridLayout(set->layout(), 4, 2, 6);
    m_ntp = new TQCheckBox(i18n("Set date and time &automatically (network time synchronization)"), set);
    sg->addMultiCellWidget(m_ntp, 0, 0, 0, 1);
    sg->addWidget(new TQLabel(i18n("Date and time:"), set), 1, 0);
    m_edit = new TQDateTimeEdit(set);
    sg->addWidget(m_edit, 1, 1, TQt::AlignLeft);
    sg->addWidget(new TQLabel(i18n("Time zone:"), set), 2, 0);
    m_zone = new TQComboBox(set);
    m_zone->setEditable(false);
    sg->addWidget(m_zone, 2, 1, TQt::AlignLeft);
    m_localRtc = new TQCheckBox(i18n("Hardware clock (RTC) runs in &local time (not recommended)"), set);
    sg->addMultiCellWidget(m_localRtc, 3, 3, 0, 1);
    sg->setColStretch(1, 1);
    top->addWidget(set);

    TQHBoxLayout *buttons = new TQHBoxLayout(top, 6);
    buttons->addStretch(1);
    KPushButton *refreshBtn = new KPushButton(SmallIconSet("reload"), i18n("&Reset"), this);
    m_apply = new KPushButton(SmallIconSet("apply"), i18n("&Apply"), this);
    buttons->addWidget(refreshBtn);
    buttons->addWidget(m_apply);
    top->addStretch(1);

    m_timer = new TQTimer(this);
    connect(m_timer, TQ_SIGNAL(timeout()), this, TQ_SLOT(slotTick()));
    connect(m_ntp, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotNtpToggled(bool)));
    connect(m_edit, TQ_SIGNAL(valueChanged(const TQDateTime &)), this, TQ_SLOT(slotChanged()));
    connect(m_zone, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotChanged()));
    connect(m_localRtc, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));
    connect(refreshBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(refresh()));
    connect(m_apply, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotApply()));
    m_apply->setEnabled(false);
}

TQString DateTimePage::title() const { return i18n("Date && Time"); }
TQString DateTimePage::iconName() const { return "date"; }

bool DateTimePage::isAvailable()
{
    return !SdProcess::findTool("timedatectl").isNull() && SdProcess::hasSystemdService("timedate1");
}

void DateTimePage::doRefresh()
{
    SdResult r = SdProcess::tool("timedatectl", SystemScope, TQStringList("show"));
    TQMap<TQString, TQString> v;
    TQStringList lines = r.lines();
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        int p = (*it).find('=');
        if (p > 0)
            v.insert((*it).left(p), (*it).mid(p + 1));
    }
    if (!r.ok()) {
        m_sync->setText(i18n("systemd-timedated cannot be reached: %1").arg(r.errorText()));
        setEnabled(true);
    }

    m_curZone = v["Timezone"];
    m_curNtp = v["NTP"] == "yes";
    m_curLocalRtc = v["LocalRTC"] == "yes";
    m_canNtp = v["CanNTP"] == "yes";
    bool synced = v["NTPSynchronized"] == "yes";

    if (m_zone->count() == 0) {
        SdResult z = SdProcess::tool("timedatectl", SystemScope, TQStringList("list-timezones") << "--no-pager");
        m_zone->insertStringList(z.lines());
    }
    for (int i = 0; i < m_zone->count(); ++i) {
        if (m_zone->text(i) == m_curZone) {
            m_zone->setCurrentItem(i);
            break;
        }
    }

    m_ntp->blockSignals(true);
    m_ntp->setChecked(m_curNtp);
    m_ntp->setEnabled(m_canNtp);
    m_ntp->blockSignals(false);
    m_localRtc->blockSignals(true);
    m_localRtc->setChecked(m_curLocalRtc);
    m_localRtc->blockSignals(false);

    m_edit->blockSignals(true);
    m_edit->setDateTime(TQDateTime::currentDateTime());
    m_edit->blockSignals(false);
    m_edit->setEnabled(!m_curNtp);
    m_timeEdited = false;

    TQString syncText;
    if (!m_canNtp)
        syncText = i18n("No network time service is installed.");
    else if (m_curNtp)
        syncText = synced ? i18n("Synchronized with a network time server.")
                          : i18n("Network time synchronization is enabled but not synchronized yet.");
    else
        syncText = i18n("Network time synchronization is disabled.");
    if (r.ok())
        m_sync->setText(syncText);

    slotTick();
    m_timer->start(1000);
    m_apply->setEnabled(false);
}

void DateTimePage::slotTick()
{
    TQDateTime now = TQDateTime::currentDateTime();
    m_clock->setText(TDEGlobal::locale()->formatDateTime(now, false, true));
    m_utc->setText(i18n("Time zone: %1    Universal time: %2")
                   .arg(m_curZone)
                   .arg(TDEGlobal::locale()->formatDateTime(TQDateTime::currentDateTime(TQt::UTC), true, true)));
    if (!m_timeEdited && !m_edit->hasFocus()) {
        m_edit->blockSignals(true);
        m_edit->setDateTime(now);
        m_edit->blockSignals(false);
    }
}

void DateTimePage::slotNtpToggled(bool on)
{
    m_edit->setEnabled(!on);
    slotChanged();
}

void DateTimePage::slotChanged()
{
    if (sender() == m_edit)
        m_timeEdited = true;
    bool changed = m_ntp->isChecked() != m_curNtp || m_localRtc->isChecked() != m_curLocalRtc ||
                   m_zone->currentText() != m_curZone || (m_timeEdited && !m_ntp->isChecked());
    m_apply->setEnabled(changed);
}

void DateTimePage::slotApply()
{
    SdResult r;
    if (m_zone->currentText() != m_curZone) {
        r = SdProcess::privileged("timedate", TQStringList("set-timezone") << m_zone->currentText());
        if (!r.ok()) {
            SdProcess::showError(this, i18n("Could not change the time zone."), r);
            refresh();
            return;
        }
    }
    if (m_localRtc->isChecked() != m_curLocalRtc) {
        r = SdProcess::privileged("timedate", TQStringList("set-local-rtc") << (m_localRtc->isChecked() ? "true" : "false"));
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not change the hardware clock mode."), r);
    }
    if (m_ntp->isChecked() != m_curNtp) {
        r = SdProcess::privileged("timedate", TQStringList("set-ntp") << (m_ntp->isChecked() ? "true" : "false"));
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not change network time synchronization."), r);
    }
    if (m_timeEdited && !m_ntp->isChecked()) {
        TQString t = m_edit->dateTime().toString("yyyy-MM-dd hh:mm:ss");
        r = SdProcess::privileged("timedate", TQStringList("set-time") << t);
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not set the system time."), r);
    }
    refresh();
}

#include "datetimepage.moc"
