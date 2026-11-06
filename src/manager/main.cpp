/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "mainwindow.h"
#include "managerwidget.h"

#include <config.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>

static TDECmdLineOptions options[] = {
    { "page <name>", I18N_NOOP("Page to show first: overview, services, timers, datetime, hostname or locale"), 0 },
    { "unit <unit>", I18N_NOOP("Show the services page and select this unit"), 0 },
    { "user", I18N_NOOP("Use the user service manager instead of the system manager for --unit"), 0 },
    TDECmdLineLastOption
};

int main(int argc, char **argv)
{
    TDEAboutData about("tde-systemd-manager", I18N_NOOP("Systemd Manager"), VERSION,
                       I18N_NOOP("Manage systemd services, timers and system settings"),
                       TDEAboutData::License_GPL_V2, "(C) 2026 tde-systemd developers", 0,
                       "https://github.com/Yjlion/tde-systemd");
    about.addAuthor("tde-systemd developers", 0, 0);
    about.setProductName("tde-systemd");

    TDECmdLineArgs::init(argc, argv, &about);
    TDECmdLineArgs::addCmdLineOptions(options);
    TDEApplication app;
    TDEGlobal::locale()->insertCatalogue("tde-systemd");

    ManagerWindow *w = new ManagerWindow();
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
    if (args->isSet("page"))
        w->manager()->showPage(TQString::fromLocal8Bit(args->getOption("page")));
    if (args->isSet("unit"))
        w->manager()->selectUnit(TQString::fromLocal8Bit(args->getOption("unit")), args->isSet("user"));
    args->clear();
    app.setMainWidget(w);
    w->show();
    return app.exec();
}
