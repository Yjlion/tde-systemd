/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "mainwindow.h"
#include "journalwidget.h"

#include <tdeaction.h>
#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdemenubar.h>
#include <tdepopupmenu.h>
#include <kstdaction.h>
#include <tdetoolbar.h>

JournalWindow::JournalWindow()
    : TDEMainWindow(0, "JournalWindow")
{
    m_journal = new JournalWidget(this);
    setCentralWidget(m_journal);

    TDEAction *reload = new TDEAction(i18n("&Refresh"), "reload", TDEShortcut(TQt::Key_F5),
                                      m_journal, TQ_SLOT(reload()), actionCollection(), "refresh");
    m_follow = new TDEToggleAction(i18n("&Follow"), "media-playback-start", TDEShortcut(TQt::CTRL + TQt::Key_F),
                                   m_journal, TQ_SLOT(toggleFollow()), actionCollection(), "follow");
    TDEAction *exportAct = new TDEAction(i18n("&Export..."), "document-save-as", TDEShortcut(TQt::CTRL + TQt::Key_S),
                                         m_journal, TQ_SLOT(exportLog()), actionCollection(), "export");
    TDEAction *clear = new TDEAction(i18n("&Clear Filters"), "edit-clear", TDEShortcut(TQt::CTRL + TQt::Key_L),
                                     m_journal, TQ_SLOT(clearFilters()), actionCollection(), "clear_filters");
    TDEAction *maint = new TDEAction(i18n("Journal &Maintenance..."), "configure", TDEShortcut(TQt::CTRL + TQt::Key_M),
                                     m_journal, TQ_SLOT(openMaintenance()), actionCollection(), "maintenance");
    TDEAction *manager = new TDEAction(i18n("Open Systemd &Manager"), "services", TDEShortcut(),
                                       this, TQ_SLOT(slotOpenManager()), actionCollection(), "manager");
    TDEAction *quit = KStdAction::quit(kapp, TQ_SLOT(quit()), actionCollection());

    // keep the toggle action in sync with the follow button of the widget
    connect(m_journal, TQ_SIGNAL(followChanged(bool)), m_follow, TQ_SLOT(setChecked(bool)));

    TDEPopupMenu *file = new TDEPopupMenu(this);
    exportAct->plug(file);
    file->insertSeparator();
    manager->plug(file);
    file->insertSeparator();
    quit->plug(file);
    menuBar()->insertItem(i18n("&File"), file);

    TDEPopupMenu *view = new TDEPopupMenu(this);
    reload->plug(view);
    m_follow->plug(view);
    clear->plug(view);
    menuBar()->insertItem(i18n("&View"), view);

    TDEPopupMenu *tools = new TDEPopupMenu(this);
    maint->plug(tools);
    menuBar()->insertItem(i18n("&Tools"), tools);
    menuBar()->insertItem(i18n("&Help"), helpMenu());

    reload->plug(toolBar());
    m_follow->plug(toolBar());
    exportAct->plug(toolBar());
    maint->plug(toolBar());

    setAutoSaveSettings("JournalWindow", true);
    if (!initialGeometrySet())
        resize(1100, 720);
}

void JournalWindow::slotOpenManager()
{
    TDEApplication::tdeinitExec("tde-systemd-manager");
}

#include "mainwindow.moc"
