/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "mainwindow.h"
#include "managerwidget.h"

#include <tdeaction.h>
#include <tdeapplication.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemenubar.h>
#include <tdepopupmenu.h>
#include <kstdaction.h>
#include <tdetoolbar.h>

ManagerWindow::ManagerWindow()
    : TDEMainWindow(0, "ManagerWindow")
{
    m_manager = new ManagerWidget(this);
    setCentralWidget(m_manager);

    TDEAction *refresh = new TDEAction(i18n("&Refresh"), "reload", TDEShortcut(TQt::Key_F5),
                                       m_manager, TQ_SLOT(refreshCurrent()), actionCollection(), "refresh");
    TDEAction *journal = new TDEAction(i18n("Open &Journal Viewer"), "toggle_log", TDEShortcut(TQt::CTRL + TQt::Key_J),
                                       this, TQ_SLOT(slotOpenJournal()), actionCollection(), "journal");
    TDEAction *quit = KStdAction::quit(kapp, TQ_SLOT(quit()), actionCollection());

    static const char *const names[] = { "show_datetime", "show_hostname", "show_locale" };
    TQString texts[] = { i18n("Show &Date && Time Page"), i18n("Show &Hostname Page"), i18n("Show &Locale && Keyboard Page") };
    for (int i = 0; i < 3; ++i) {
        ManagerWidget::OptionalPage p = (ManagerWidget::OptionalPage)i;
        m_pageActions[i] = new TDEToggleAction(texts[i], TDEShortcut(), this, TQ_SLOT(slotTogglePage()),
                                               actionCollection(), names[i]);
        m_pageActions[i]->setChecked(ManagerWidget::pageEnabled(p));
        m_pageActions[i]->setEnabled(ManagerWidget::pageAvailable(p));
        if (!ManagerWidget::pageAvailable(p))
            m_pageActions[i]->setToolTip(i18n("The required systemd service is not installed"));
    }

    TDEPopupMenu *file = new TDEPopupMenu(this);
    refresh->plug(file);
    journal->plug(file);
    file->insertSeparator();
    quit->plug(file);
    menuBar()->insertItem(i18n("&File"), file);

    TDEPopupMenu *settings = new TDEPopupMenu(this);
    for (int i = 0; i < 3; ++i)
        m_pageActions[i]->plug(settings);
    menuBar()->insertItem(i18n("&Settings"), settings);
    menuBar()->insertItem(i18n("&Help"), helpMenu());

    refresh->plug(toolBar());
    journal->plug(toolBar());

    setAutoSaveSettings("ManagerWindow", true);
    if (!initialGeometrySet())
        resize(980, 700);
}

void ManagerWindow::slotTogglePage()
{
    for (int i = 0; i < 3; ++i)
        ManagerWidget::setPageEnabled((ManagerWidget::OptionalPage)i, m_pageActions[i]->isChecked());
    m_manager->rebuild();
}

void ManagerWindow::slotOpenJournal()
{
    TDEApplication::tdeinitExec("tde-journal-viewer");
}

#include "mainwindow.moc"
