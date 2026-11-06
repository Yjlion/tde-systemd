/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_TIMERSPAGE_H
#define TDESYSTEMD_TIMERSPAGE_H

#include "sdpage.h"

class TQComboBox;
class TQTextEdit;
class KPushButton;
class TDEListView;

/** Overview of timer units, when they ran and when they run next. */
class TimersPage : public SdPage
{
    TQ_OBJECT

public:
    TimersPage(TQWidget *parent = 0, const char *name = 0);
    TQString title() const;
    TQString iconName() const;

protected:
    void doRefresh();

private slots:
    void slotSelectionChanged();
    void slotStart();
    void slotStop();
    void slotEnable();
    void slotDisable();

private:
    void act(const TQString &verb);
    TQString currentTimer() const;

    TQComboBox *m_scope;
    TDEListView *m_list;
    TQTextEdit *m_status;
    KPushButton *m_start, *m_stop, *m_enable, *m_disable;
};

#endif
