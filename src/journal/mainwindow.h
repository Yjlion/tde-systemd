/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_JOURNAL_MAINWINDOW_H
#define TDESYSTEMD_JOURNAL_MAINWINDOW_H

#include <tdemainwindow.h>

class TDEToggleAction;
class JournalWidget;

class JournalWindow : public TDEMainWindow
{
    TQ_OBJECT

public:
    JournalWindow();
    JournalWidget *journal() const { return m_journal; }

private slots:
    void slotOpenManager();

private:
    JournalWidget *m_journal;
    TDEToggleAction *m_follow;
};

#endif
