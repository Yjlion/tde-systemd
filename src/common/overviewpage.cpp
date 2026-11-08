/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "overviewpage.h"
#include "sdprocess.h"
#include "sdunits.h"

#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqregexp.h>
#include <tqsplitter.h>
#include <tqtextedit.h>

#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <kpushbutton.h>

static const char *const s_rows[] = {
    I18N_NOOP("Hostname"), I18N_NOOP("Operating System"), I18N_NOOP("Kernel"),
    I18N_NOOP("Architecture"), I18N_NOOP("Hardware"), I18N_NOOP("Virtualization"),
    I18N_NOOP("systemd Version"), I18N_NOOP("Startup Time"), I18N_NOOP("Units"), 0
};

OverviewPage::OverviewPage(TQWidget *parent, const char *name)
    : SdPage(parent, name)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);

    TQHBoxLayout *head = new TQHBoxLayout(top, 10);
    m_stateIcon = new TQLabel(this);
    head->addWidget(m_stateIcon);
    m_state = new TQLabel(this);
    TQFont f = m_state->font();
    f.setPointSize(f.pointSize() + 4);
    f.setBold(true);
    m_state->setFont(f);
    head->addWidget(m_state, 1);
    KPushButton *refreshBtn = new KPushButton(SmallIconSet("reload"), i18n("&Refresh"), this);
    head->addWidget(refreshBtn);

    TQGroupBox *info = new TQGroupBox(i18n("System Information"), this);
    info->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *grid = new TQGridLayout(info->layout(), 1, 2, 6);
    grid->setColStretch(1, 1);
    for (int i = 0; s_rows[i]; ++i)
        m_values.insert(s_rows[i], addRow(info, grid, i, i18n(s_rows[i])));
    top->addWidget(info);

    TQSplitter *split = new TQSplitter(TQt::Horizontal, this);
    top->addWidget(split, 1);

    TQGroupBox *failedBox = new TQGroupBox(i18n("Failed Units"), split);
    failedBox->setColumnLayout(0, TQt::Vertical);
    TQVBoxLayout *fl = new TQVBoxLayout(failedBox->layout(), 6);
    m_failed = new TDEListView(failedBox);
    m_failed->addColumn(i18n("Unit"));
    m_failed->addColumn(i18n("Scope"));
    m_failed->addColumn(i18n("Description"));
    m_failed->setAllColumnsShowFocus(true);
    fl->addWidget(m_failed);
    TQHBoxLayout *fb = new TQHBoxLayout(fl, 6);
    KPushButton *reset = new KPushButton(SmallIconSet("edit-undo"), i18n("Reset &Failed Units"), failedBox);
    KPushButton *reload = new KPushButton(SmallIconSet("rebuild"), i18n("Reload &Daemon"), failedBox);
    fb->addWidget(reset);
    fb->addWidget(reload);
    fb->addStretch(1);

    TQGroupBox *blameBox = new TQGroupBox(i18n("Slowest Units During Startup"), split);
    blameBox->setColumnLayout(0, TQt::Vertical);
    TQVBoxLayout *bl = new TQVBoxLayout(blameBox->layout(), 6);
    m_blame = sdCreateTextView(blameBox);
    bl->addWidget(m_blame);

    connect(refreshBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(refresh()));
    connect(reset, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotResetFailed()));
    connect(reload, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotDaemonReload()));
}

TQLabel *OverviewPage::addRow(TQWidget *parent, TQGridLayout *grid, int row, const TQString &label)
{
    TQLabel *l = new TQLabel(label + ":", parent);
    TQFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    grid->addWidget(l, row, 0, TQt::AlignRight | TQt::AlignTop);
    TQLabel *v = new TQLabel(parent);
    v->setTextFormat(TQt::PlainText);
    grid->addWidget(v, row, 1);
    return v;
}

TQString OverviewPage::title() const { return i18n("Overview"); }
TQString OverviewPage::iconName() const { return "computer"; }

static TQMap<TQString, TQString> parseKeyValues(const TQStringList &lines, TQChar sep)
{
    TQMap<TQString, TQString> m;
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        int p = (*it).find(sep);
        if (p > 0)
            m.insert((*it).left(p).stripWhiteSpace(), sdStripSymbols((*it).mid(p + 1)));
    }
    return m;
}

void OverviewPage::doRefresh()
{
    // system state
    SdResult r = SdProcess::tool("systemctl", SystemScope, TQStringList("is-system-running"));
    TQString state = r.output().stripWhiteSpace();
    TQString icon = "messagebox_info";
    TQString text;
    if (state == "running") {
        icon = "button_ok";
        text = i18n("The system is running normally");
    } else if (state == "degraded") {
        icon = "messagebox_warning";
        text = i18n("The system is degraded: one or more units failed");
    } else if (state == "starting" || state == "initializing") {
        icon = "reload";
        text = i18n("The system is still starting up");
    } else if (state == "stopping") {
        text = i18n("The system is shutting down");
    } else if (state == "maintenance") {
        icon = "messagebox_critical";
        text = i18n("The system is in maintenance mode");
    } else {
        icon = "messagebox_critical";
        text = state.isEmpty() ? i18n("systemd is not running or cannot be reached") : state;
    }
    m_stateIcon->setPixmap(DesktopIcon(icon, 48));
    m_state->setText(text);

    // host information
    TQMap<TQString, TQString> host = parseKeyValues(SdProcess::tool("hostnamectl", SystemScope, TQStringList("status")).lines(), ':');
    m_values["Hostname"]->setText(host["Static hostname"].isEmpty() ? host["Transient hostname"] : host["Static hostname"]);
    m_values["Operating System"]->setText(host["Operating System"]);
    m_values["Kernel"]->setText(host["Kernel"]);
    m_values["Architecture"]->setText(host["Architecture"]);
    TQString hw = (host["Hardware Vendor"] + " " + host["Hardware Model"]).stripWhiteSpace();
    m_values["Hardware"]->setText(hw.isEmpty() ? host["Chassis"] : hw);
    m_values["Virtualization"]->setText(host["Virtualization"].isEmpty() ? i18n("none") : host["Virtualization"]);

    r = SdProcess::tool("systemctl", SystemScope, TQStringList("--version"));
    m_values["systemd Version"]->setText(r.lines().isEmpty() ? TQString::null : r.lines().first());

    r = SdProcess::run(TQStringList("systemd-analyze") << "time");
    TQString startup = r.output().simplifyWhiteSpace();
    startup.replace("Startup finished in ", "");
    m_values["Startup Time"]->setText(r.ok() ? startup : r.errorText());

    // failed units
    m_failed->clear();
    int sysFailed = 0, userFailed = 0;
    for (int s = 0; s < 2; ++s) {
        SdScope scope = s ? UserScope : SystemScope;
        r = SdProcess::tool("systemctl", scope, TQStringList("list-units") << "--failed" << "--plain" << "--no-legend" << "--no-pager");
        TQStringList lines = r.lines();
        for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
            TQString line = (*it).stripWhiteSpace();
            if (line.startsWith(TQString::fromUtf8("\xe2\x97\x8f")))
                line = line.mid(1).stripWhiteSpace();
            TQStringList f = TQStringList::split(' ', line.simplifyWhiteSpace());
            if (f.count() < 4)
                continue;
            TDEListViewItem *item = new TDEListViewItem(m_failed, f[0], s ? i18n("User") : i18n("System"),
                                                        line.section(TQRegExp("\\s+"), 4));
            item->setPixmap(0, SmallIcon("button_cancel"));
            if (s)
                ++userFailed;
            else
                ++sysFailed;
        }
    }

    // unit statistics
    r = SdProcess::tool("systemctl", SystemScope, TQStringList("list-units") << "--plain" << "--no-legend" << "--no-pager" << "--state=active");
    int activeUnits = r.lines().count();
    m_values["Units"]->setText(i18n("%1 active, %2 failed (system), %3 failed (user)")
                               .arg(activeUnits).arg(sysFailed).arg(userFailed));

    r = SdProcess::run(TQStringList("systemd-analyze") << "blame" << "--no-pager");
    TQStringList blame = r.lines();
    while (blame.count() > 40)
        blame.remove(blame.fromLast());
    m_blame->setText(r.ok() ? blame.join("\n") : r.errorText());
}

void OverviewPage::slotResetFailed()
{
    SdUnits::managerAction(this, SystemScope, "reset-failed");
    SdUnits::managerAction(this, UserScope, "reset-failed");
    refresh();
}

void OverviewPage::slotDaemonReload()
{
    SdUnits::managerAction(this, SystemScope, "daemon-reload");
    refresh();
}

#include "overviewpage.moc"
