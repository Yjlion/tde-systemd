/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_DATETIMEPAGE_H
#define TDESYSTEMD_DATETIMEPAGE_H

#include "sdpage.h"

class TQCheckBox;
class TQComboBox;
class TQDateTimeEdit;
class TQLabel;
class TQTimer;
class KPushButton;

/** Date, time, time zone and NTP settings through systemd-timedated. */
class DateTimePage : public SdPage
{
    TQ_OBJECT

public:
    DateTimePage(TQWidget *parent = 0, const char *name = 0);
    TQString title() const;
    TQString iconName() const;

    /** True if systemd-timedated is installed on this system. */
    static bool isAvailable();

protected:
    void doRefresh();

private slots:
    void slotTick();
    void slotNtpToggled(bool on);
    void slotChanged();
    void slotApply();

private:
    TQLabel *m_clock;
    TQLabel *m_utc;
    TQLabel *m_sync;
    TQCheckBox *m_ntp;
    TQDateTimeEdit *m_edit;
    TQComboBox *m_zone;
    TQCheckBox *m_localRtc;
    KPushButton *m_apply;
    TQTimer *m_timer;

    TQString m_curZone;
    bool m_curNtp;
    bool m_curLocalRtc;
    bool m_canNtp;
    bool m_timeEdited;
};

#endif
