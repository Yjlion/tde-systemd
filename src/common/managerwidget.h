/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_MANAGERWIDGET_H
#define TDESYSTEMD_MANAGERWIDGET_H

#include <tqwidget.h>
#include <tqptrlist.h>

class TQTabWidget;
class SdPage;

/**
 * The systemd manager: a tab widget with the overview, services, timers and
 * the optional date & time, hostname and locale pages. Used by the
 * stand-alone application and by the TDE Control Center module.
 */
class ManagerWidget : public TQWidget
{
    TQ_OBJECT

public:
    enum OptionalPage { DateTime = 0, Hostname, Locale };

    ManagerWidget(TQWidget *parent = 0, const char *name = 0);

    /** Whether an optional page is enabled in the configuration. */
    static bool pageEnabled(OptionalPage page);
    static void setPageEnabled(OptionalPage page, bool on);
    /** Whether the service behind an optional page exists on this system. */
    static bool pageAvailable(OptionalPage page);

    void showPage(const TQString &name);

    /** Switch to the services page and select @p unit. */
    void selectUnit(const TQString &unit, bool user);

public slots:
    /** Recreate the pages, e.g. after the configuration changed. */
    void rebuild();
    void refreshCurrent();

private slots:
    void slotCurrentChanged(TQWidget *);

private:
    TQTabWidget *m_tabs;
    TQPtrList<SdPage> m_pages;
};

#endif
