/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 *
 * TDE Control Center modules wrapping the systemd manager and the journal viewer.
 */

#include "managerwidget.h"
#include "journalwidget.h"

#include <config.h>

#include <tqlayout.h>

#include <tdeaboutdata.h>
#include <tdecmodule.h>
#include <tdeglobal.h>
#include <tdelocale.h>

class SystemdModule : public TDECModule
{
public:
    SystemdModule(TQWidget *parent, const char *name)
        : TDECModule(parent, name)
    {
        TDEGlobal::locale()->insertCatalogue("tde-systemd");
        TQVBoxLayout *l = new TQVBoxLayout(this, 0, 0);
        l->addWidget(new ManagerWidget(this));
        setButtons(Help);
        setQuickHelp(i18n("<h1>Systemd</h1> Manage the services and timers of the system and of your "
                          "user session, and change the date and time, the hostname and the system locale. "
                          "Changes to the system require administrator authentication through polkit."));
        TDEAboutData *about = new TDEAboutData("kcm_tdesystemd", I18N_NOOP("Systemd Manager"), VERSION,
                                               I18N_NOOP("Manage systemd services and system settings"),
                                               TDEAboutData::License_GPL_V2, "(C) 2026 tde-systemd developers");
        setAboutData(about);
    }
};

class JournalModule : public TDECModule
{
public:
    JournalModule(TQWidget *parent, const char *name)
        : TDECModule(parent, name)
    {
        TDEGlobal::locale()->insertCatalogue("tde-systemd");
        TQVBoxLayout *l = new TQVBoxLayout(this, 0, 0);
        l->addWidget(new JournalWidget(this));
        setButtons(Help);
        setQuickHelp(i18n("<h1>System Journal</h1> View the messages logged by the system and by your "
                          "user session, filter them and perform journal maintenance such as vacuuming "
                          "old entries."));
        TDEAboutData *about = new TDEAboutData("kcm_tdejournal", I18N_NOOP("Journal Viewer"), VERSION,
                                               I18N_NOOP("View and maintain the systemd journal"),
                                               TDEAboutData::License_GPL_V2, "(C) 2026 tde-systemd developers");
        setAboutData(about);
    }
};

extern "C"
{
    TDE_EXPORT TDECModule *create_tdesystemd(TQWidget *parent, const char *name)
    {
        return new SystemdModule(parent, name);
    }

    TDE_EXPORT TDECModule *create_tdejournal(TQWidget *parent, const char *name)
    {
        return new JournalModule(parent, name);
    }
}
