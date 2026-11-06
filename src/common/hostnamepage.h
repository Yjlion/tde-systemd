/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_HOSTNAMEPAGE_H
#define TDESYSTEMD_HOSTNAMEPAGE_H

#include "sdpage.h"

#include <tqmap.h>

class TQComboBox;
class TQLabel;
class KLineEdit;
class KPushButton;

/** Hostname and machine information through systemd-hostnamed. */
class HostnamePage : public SdPage
{
    TQ_OBJECT

public:
    HostnamePage(TQWidget *parent = 0, const char *name = 0);
    TQString title() const;
    TQString iconName() const;

    static bool isAvailable();

protected:
    void doRefresh();

private slots:
    void slotChanged();
    void slotApply();

private:
    TQString current(const TQString &what) const;

    KLineEdit *m_static;
    KLineEdit *m_pretty;
    KLineEdit *m_icon;
    TQComboBox *m_chassis;
    KLineEdit *m_deployment;
    KLineEdit *m_location;
    TQLabel *m_info;
    KPushButton *m_apply;
    TQMap<TQString, TQString> m_cur;
};

#endif
