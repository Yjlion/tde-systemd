/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_MANAGER_MAINWINDOW_H
#define TDESYSTEMD_MANAGER_MAINWINDOW_H

#include <tdemainwindow.h>

class TDEToggleAction;
class ManagerWidget;

class ManagerWindow : public TDEMainWindow
{
    TQ_OBJECT

public:
    ManagerWindow();
    ManagerWidget *manager() const { return m_manager; }

private slots:
    void slotTogglePage();
    void slotOpenJournal();

private:
    ManagerWidget *m_manager;
    TDEToggleAction *m_pageActions[3];
};

#endif
