/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "mainwindow.h"
#include "journalwidget.h"

#include <config.h>

#include <tqtimer.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>

static TDECmdLineOptions options[] = {
    { "user", I18N_NOOP("Show the journal of the user session instead of the system journal"), 0 },
    { "u", 0, 0 },
    { "unit <unit>", I18N_NOOP("Show only messages of this unit"), 0 },
    { "p", 0, 0 },
    { "priority <0-7>", I18N_NOOP("Show only messages of this priority or higher (0 = emergency, 7 = debug)"), 0 },
    { "b", 0, 0 },
    { "boot <offset>", I18N_NOOP("Show messages of this boot (0 = current, -1 = previous, all)"), 0 },
    { "f", 0, 0 },
    { "follow", I18N_NOOP("Follow new messages"), 0 },
    { "maintenance", I18N_NOOP("Open the journal maintenance dialog"), 0 },
    TDECmdLineLastOption
};

int main(int argc, char **argv)
{
    TDEAboutData about("tde-journal-viewer", I18N_NOOP("Journal Viewer"), VERSION,
                       I18N_NOOP("View and maintain the systemd journal"),
                       TDEAboutData::License_GPL_V2, "(C) 2026 tde-systemd developers", 0,
                       "https://github.com/Yjlion/tde-systemd");
    about.addAuthor("tde-systemd developers", 0, 0);
    about.setProductName("tde-systemd");

    TDECmdLineArgs::init(argc, argv, &about);
    TDECmdLineArgs::addCmdLineOptions(options);
    TDEApplication app;
    TDEGlobal::locale()->insertCatalogue("tde-systemd");

    JournalWindow *w = new JournalWindow();
    JournalWidget *j = w->journal();
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
    if (args->isSet("user"))
        j->setScope(UserScope);
    if (args->isSet("unit"))
        j->setUnitFilter(TQString::fromLocal8Bit(args->getOption("unit")));
    if (args->isSet("priority"))
        j->setPriorityFilter(TQString(args->getOption("priority")).toInt());
    if (args->isSet("boot"))
        j->setBootFilter(TQString::fromLocal8Bit(args->getOption("boot")));
    if (args->isSet("follow"))
        j->setFollow(true);
    bool maintenance = args->isSet("maintenance");
    args->clear();

    app.setMainWidget(w);
    w->show();
    if (maintenance)
        TQTimer::singleShot(500, j, TQ_SLOT(openMaintenance()));
    return app.exec();
}
