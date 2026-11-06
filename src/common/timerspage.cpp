/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "timerspage.h"
#include "sdjson.h"
#include "sdunits.h"

#include <tqcombobox.h>
#include <tqdatetime.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqsplitter.h>
#include <tqtextedit.h>

#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <kpushbutton.h>

#include <sys/time.h>

TimersPage::TimersPage(TQWidget *parent, const char *name)
    : SdPage(parent, name)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);
    TQHBoxLayout *bar = new TQHBoxLayout(top, 6);
    bar->addWidget(new TQLabel(i18n("Scope:"), this));
    m_scope = new TQComboBox(this);
    m_scope->insertItem(SmallIcon("computer"), i18n("System"));
    m_scope->insertItem(SmallIcon("go-home"), i18n("User"));
    bar->addWidget(m_scope);
    bar->addStretch(1);
    KPushButton *refreshBtn = new KPushButton(SmallIconSet("reload"), i18n("&Refresh"), this);
    bar->addWidget(refreshBtn);

    TQSplitter *split = new TQSplitter(TQt::Vertical, this);
    top->addWidget(split, 1);
    m_list = new TDEListView(split);
    m_list->addColumn(i18n("Timer"));
    m_list->addColumn(i18n("Activates"));
    m_list->addColumn(i18n("Next Run"));
    m_list->addColumn(i18n("Left"));
    m_list->addColumn(i18n("Last Run"));
    m_list->addColumn(i18n("Passed"));
    m_list->setAllColumnsShowFocus(true);
    m_list->setShowSortIndicator(true);
    m_status = sdCreateTextView(split);

    TQHBoxLayout *buttons = new TQHBoxLayout(top, 6);
    m_start = new KPushButton(SmallIconSet("media-playback-start"), i18n("&Start"), this);
    m_stop = new KPushButton(SmallIconSet("media-playback-stop"), i18n("S&top"), this);
    m_enable = new KPushButton(SmallIconSet("button_ok"), i18n("&Enable"), this);
    m_disable = new KPushButton(SmallIconSet("button_cancel"), i18n("&Disable"), this);
    buttons->addWidget(m_start);
    buttons->addWidget(m_stop);
    buttons->addWidget(m_enable);
    buttons->addWidget(m_disable);
    buttons->addStretch(1);

    connect(m_scope, TQ_SIGNAL(activated(int)), this, TQ_SLOT(refresh()));
    connect(refreshBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(refresh()));
    connect(m_list, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(slotSelectionChanged()));
    connect(m_start, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotStart()));
    connect(m_stop, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotStop()));
    connect(m_enable, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotEnable()));
    connect(m_disable, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotDisable()));
    slotSelectionChanged();
}

TQString TimersPage::title() const { return i18n("Timers"); }
TQString TimersPage::iconName() const { return "clock"; }

void TimersPage::doRefresh()
{
    SdScope scope = m_scope->currentItem() == 1 ? UserScope : SystemScope;
    TQString keep = currentTimer();
    m_list->clear();
    SdResult r = SdProcess::tool("systemctl", scope,
                                 TQStringList("list-timers") << "--all" << "--no-pager" << "-o" << "json");
    bool ok = false;
    SdJson json = SdJson::parse(r.out, &ok);
    if (!r.ok() || !ok || !json.isArray()) {
        m_status->setText(i18n("Could not list timers:\n%1").arg(r.errorText()));
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, 0);
    long long now = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;

    const SdJson::List &timers = json.array();
    for (SdJson::List::const_iterator it = timers.begin(); it != timers.end(); ++it) {
        unsigned long long next = (*it)["next"].toULongLong();
        unsigned long long last = (*it)["last"].toULongLong();
        TDEListViewItem *item = new TDEListViewItem(m_list, (*it).str("unit"), (*it).str("activates"));
        item->setPixmap(0, SmallIcon("clock"));
        item->setText(2, next ? SdUnits::formatUsec(next) : i18n("n/a"));
        item->setText(3, next ? SdUnits::formatDuration((long long)next - now) : TQString::null);
        item->setText(4, last ? SdUnits::formatUsec(last) : i18n("never"));
        item->setText(5, last ? SdUnits::formatDuration((long long)last - now) : TQString::null);
        if (item->text(0) == keep)
            m_list->setSelected(item, true);
    }
    slotSelectionChanged();
}

TQString TimersPage::currentTimer() const
{
    TQListViewItem *it = m_list->selectedItem();
    return it ? it->text(0) : TQString::null;
}

void TimersPage::slotSelectionChanged()
{
    TQString t = currentTimer();
    m_start->setEnabled(!t.isEmpty());
    m_stop->setEnabled(!t.isEmpty());
    m_enable->setEnabled(!t.isEmpty());
    m_disable->setEnabled(!t.isEmpty());
    if (t.isEmpty()) {
        m_status->setText(i18n("Select a timer to see its status."));
        return;
    }
    SdScope scope = m_scope->currentItem() == 1 ? UserScope : SystemScope;
    SdResult r = SdProcess::tool("systemctl", scope, TQStringList("status") << "--no-pager" << "--full" << "--" << t);
    m_status->setText(r.output().isEmpty() ? r.errorText() : r.output());
}

void TimersPage::act(const TQString &verb)
{
    TQString t = currentTimer();
    if (t.isEmpty())
        return;
    SdUnits::action(this, m_scope->currentItem() == 1 ? UserScope : SystemScope, verb, t);
    refresh();
}

void TimersPage::slotStart() { act("start"); }
void TimersPage::slotStop() { act("stop"); }
void TimersPage::slotEnable() { act("enable"); }
void TimersPage::slotDisable() { act("disable"); }

#include "timerspage.moc"
