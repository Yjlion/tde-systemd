/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "servicespage.h"

#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqsplitter.h>
#include <tqtabwidget.h>
#include <tqtextedit.h>
#include <tqtimer.h>

#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelistviewsearchline.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>
#include <kpushbutton.h>

namespace {

enum Columns { ColUnit = 0, ColLoad, ColActive, ColSub, ColFile, ColDescription };

class UnitItem : public TDEListViewItem
{
public:
    UnitItem(TDEListView *parent, const SdUnit &u)
        : TDEListViewItem(parent, u.name, u.load, u.active, u.sub, u.fileState, u.description), unit(u)
    {
        TQString icon = "unknown";
        if (u.active == "active")
            icon = "button_ok";
        else if (u.active == "failed")
            icon = "button_cancel";
        else if (u.active == "activating" || u.active == "deactivating" || u.active == "reloading")
            icon = "reload";
        else if (u.fileState.startsWith("masked"))
            icon = "system-lock-screen";
        else if (u.load == "not-found")
            icon = "help-about";
        else
            icon = "media-playback-stop";
        setPixmap(ColUnit, SmallIcon(icon));
    }

    void paintCell(TQPainter *p, const TQColorGroup &cg, int column, int width, int align)
    {
        TQColorGroup g(cg);
        if (unit.active == "failed" || unit.load == "error" || unit.load == "bad-setting")
            g.setColor(TQColorGroup::Text, TQt::red);
        else if (unit.fileState.startsWith("masked") || unit.load == "not-found")
            g.setColor(TQColorGroup::Text, TQt::darkGray);
        TDEListViewItem::paintCell(p, g, column, width, align);
    }

    int compare(TQListViewItem *other, int col, bool asc) const
    {
        return text(col).lower().localeAwareCompare(other->text(col).lower());
    }

    SdUnit unit;
};

enum ActionIds { ActStart = 1, ActStop, ActRestart, ActReload, ActEnable, ActDisable,
                 ActMask, ActUnmask, ActResetFailed, ActKill };

} // namespace

ServicesPage::ServicesPage(TQWidget *parent, const char *name)
    : SdPage(parent, name)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);

    // ---- filter bar
    TQHBoxLayout *bar = new TQHBoxLayout(top, 6);
    bar->addWidget(new TQLabel(i18n("Scope:"), this));
    m_scope = new TQComboBox(this);
    m_scope->insertItem(SmallIcon("computer"), i18n("System"));
    m_scope->insertItem(SmallIcon("go-home"), i18n("User"));
    bar->addWidget(m_scope);

    bar->addWidget(new TQLabel(i18n("Type:"), this));
    m_type = new TQComboBox(this);
    m_type->insertItem(i18n("Services"));
    m_type->insertItem(i18n("Sockets"));
    m_type->insertItem(i18n("Timers"));
    m_type->insertItem(i18n("Targets"));
    m_type->insertItem(i18n("Mounts"));
    m_type->insertItem(i18n("Paths"));
    m_type->insertItem(i18n("Slices"));
    m_type->insertItem(i18n("All Units"));
    bar->addWidget(m_type);

    bar->addWidget(new TQLabel(i18n("State:"), this));
    m_state = new TQComboBox(this);
    m_state->insertItem(i18n("All"));
    m_state->insertItem(i18n("Active"));
    m_state->insertItem(i18n("Inactive"));
    m_state->insertItem(i18n("Failed"));
    m_state->insertItem(i18n("Enabled"));
    m_state->insertItem(i18n("Disabled"));
    m_state->insertItem(i18n("Static"));
    m_state->insertItem(i18n("Masked"));
    bar->addWidget(m_state);

    bar->addWidget(new TQLabel(i18n("Search:"), this));
    m_search = new TDEListViewSearchLine(this);
    bar->addWidget(m_search, 1);

    KPushButton *refreshBtn = new KPushButton(SmallIconSet("reload"), i18n("&Refresh"), this);
    bar->addWidget(refreshBtn);

    // ---- list and details
    TQSplitter *split = new TQSplitter(TQt::Vertical, this);
    top->addWidget(split, 1);

    m_list = new TDEListView(split);
    m_list->addColumn(i18n("Unit"));
    m_list->addColumn(i18n("Load"));
    m_list->addColumn(i18n("Active"));
    m_list->addColumn(i18n("Sub"));
    m_list->addColumn(i18n("Enabled"));
    m_list->addColumn(i18n("Description"));
    m_list->setAllColumnsShowFocus(true);
    m_list->setShowSortIndicator(true);
    m_list->setSelectionMode(TQListView::Single);
    m_list->setColumnWidthMode(ColDescription, TQListView::Maximum);
    m_search->setListView(m_list);

    m_details = new TQTabWidget(split);
    m_status = sdCreateTextView(m_details);
    m_file = sdCreateTextView(m_details);
    m_props = sdCreateTextView(m_details);
    m_deps = sdCreateTextView(m_details);
    m_log = sdCreateTextView(m_details);
    m_details->addTab(m_status, SmallIconSet("info"), i18n("Status"));
    m_details->addTab(m_file, SmallIconSet("text-x-generic"), i18n("Unit File"));
    m_details->addTab(m_props, SmallIconSet("configure"), i18n("Properties"));
    m_details->addTab(m_deps, SmallIconSet("view_tree"), i18n("Dependencies"));
    m_details->addTab(m_log, SmallIconSet("toggle_log"), i18n("Log"));

    TQValueList<int> sizes;
    sizes << 300 << 160;
    split->setSizes(sizes);

    // ---- action buttons
    TQHBoxLayout *buttons = new TQHBoxLayout(top, 6);
    m_start = new KPushButton(SmallIconSet("media-playback-start"), i18n("&Start"), this);
    m_stop = new KPushButton(SmallIconSet("media-playback-stop"), i18n("S&top"), this);
    m_restart = new KPushButton(SmallIconSet("reload"), i18n("Res&tart"), this);
    m_reload = new KPushButton(SmallIconSet("reload_page"), i18n("Re&load"), this);
    m_enable = new KPushButton(SmallIconSet("button_ok"), i18n("&Enable"), this);
    m_disable = new KPushButton(SmallIconSet("button_cancel"), i18n("&Disable"), this);
    m_mask = new KPushButton(SmallIconSet("system-lock-screen"), i18n("&Mask"), this);
    m_unmask = new KPushButton(SmallIconSet("unlock"), i18n("&Unmask"), this);
    m_daemonReload = new KPushButton(SmallIconSet("rebuild"), i18n("Reload Daemon"), this);
    buttons->addWidget(m_start);
    buttons->addWidget(m_stop);
    buttons->addWidget(m_restart);
    buttons->addWidget(m_reload);
    buttons->addSpacing(12);
    buttons->addWidget(m_enable);
    buttons->addWidget(m_disable);
    buttons->addWidget(m_mask);
    buttons->addWidget(m_unmask);
    buttons->addStretch(1);
    m_summary = new TQLabel(this);
    buttons->addWidget(m_summary);
    buttons->addSpacing(12);
    buttons->addWidget(m_daemonReload);

    connect(m_scope, TQ_SIGNAL(activated(int)), this, TQ_SLOT(refresh()));
    connect(m_type, TQ_SIGNAL(activated(int)), this, TQ_SLOT(refresh()));
    connect(m_state, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotFilterChanged()));
    connect(refreshBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(refresh()));
    connect(m_list, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(slotSelectionChanged()));
    connect(m_list, TQ_SIGNAL(contextMenu(TDEListView *, TQListViewItem *, const TQPoint &)),
            this, TQ_SLOT(slotContextMenu(TDEListView *, TQListViewItem *, const TQPoint &)));
    connect(m_details, TQ_SIGNAL(currentChanged(TQWidget *)), this, TQ_SLOT(slotDetailTabChanged()));
    connect(m_start, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotStart()));
    connect(m_stop, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotStop()));
    connect(m_restart, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotRestart()));
    connect(m_reload, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotReload()));
    connect(m_enable, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotEnable()));
    connect(m_disable, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotDisable()));
    connect(m_mask, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotMask()));
    connect(m_unmask, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotUnmask()));
    connect(m_daemonReload, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotDaemonReload()));

    updateButtons();
}

TQString ServicesPage::title() const
{
    return i18n("Services");
}

TQString ServicesPage::iconName() const
{
    return "services";
}

void ServicesPage::selectUnit(const TQString &unit, SdScope s)
{
    m_scope->setCurrentItem(s == UserScope ? 1 : 0);
    if (!unit.endsWith(".service"))
        m_type->setCurrentItem(m_type->count() - 1);
    m_pendingSelect = unit;
    markLoaded();
    refresh();
}

SdScope ServicesPage::scope() const
{
    return m_scope->currentItem() == 1 ? UserScope : SystemScope;
}

void ServicesPage::doRefresh()
{
    static const char *const types[] = { "service", "socket", "timer", "target", "mount", "path", "slice", "" };
    TQString keep = m_pendingSelect.isEmpty() ? currentUnit() : m_pendingSelect;
    m_pendingSelect = TQString::null;

    TQString error;
    m_units = SdUnits::list(scope(), types[m_type->currentItem()], &error);
    if (!error.isEmpty() && m_units.isEmpty()) {
        m_list->clear();
        m_summary->setText(TQString::null);
        m_status->setText(i18n("Could not list units:\n%1").arg(error));
        return;
    }
    fillList();

    if (!keep.isEmpty()) {
        TQListViewItem *it = m_list->findItem(keep, ColUnit);
        if (it) {
            m_list->setSelected(it, true);
            m_list->ensureItemVisible(it);
        }
    }
    m_detailsFor = TQString::null;
    loadDetails();
}

void ServicesPage::fillList()
{
    m_list->setUpdatesEnabled(false);
    m_list->clear();
    int state = m_state->currentItem();
    int shown = 0, active = 0, failed = 0;
    for (SdUnitList::ConstIterator it = m_units.begin(); it != m_units.end(); ++it) {
        const SdUnit &u = *it;
        bool show = true;
        switch (state) {
        case 1: show = u.active == "active"; break;
        case 2: show = u.active == "inactive"; break;
        case 3: show = u.active == "failed"; break;
        case 4: show = u.fileState.startsWith("enabled"); break;
        case 5: show = u.fileState == "disabled"; break;
        case 6: show = u.fileState == "static"; break;
        case 7: show = u.fileState.startsWith("masked"); break;
        default: break;
        }
        if (u.active == "active")
            ++active;
        if (u.active == "failed")
            ++failed;
        if (!show)
            continue;
        new UnitItem(m_list, u);
        ++shown;
    }
    m_list->setUpdatesEnabled(true);
    m_list->triggerUpdate();
    m_search->updateSearch();
    m_summary->setText(i18n("%1 shown, %2 active, %3 failed").arg(shown).arg(active).arg(failed));
    updateButtons();
}

void ServicesPage::slotFilterChanged()
{
    fillList();
}

TQString ServicesPage::currentUnit() const
{
    TQListViewItem *it = m_list->selectedItem();
    return it ? it->text(ColUnit) : TQString::null;
}

void ServicesPage::updateButtons()
{
    UnitItem *it = static_cast<UnitItem *>(m_list->selectedItem());
    bool sel = it != 0;
    bool isTemplate = sel && it->unit.name.contains("@.");
    bool active = sel && (it->unit.active == "active" || it->unit.active == "activating" ||
                          it->unit.active == "reloading");
    bool masked = sel && it->unit.fileState.startsWith("masked");
    TQString fs = sel ? it->unit.fileState : TQString::null;
    bool installable = fs == "enabled" || fs == "disabled" || fs == "indirect" ||
                       fs == "enabled-runtime" || fs == "alias";

    m_start->setEnabled(sel && !isTemplate && !active && !masked);
    m_stop->setEnabled(sel && !isTemplate && active);
    m_restart->setEnabled(sel && !isTemplate && !masked);
    m_reload->setEnabled(sel && !isTemplate && active);
    m_enable->setEnabled(sel && installable && fs != "enabled");
    m_disable->setEnabled(sel && installable && fs != "disabled");
    m_mask->setEnabled(sel && !isTemplate && !masked);
    m_unmask->setEnabled(sel && masked);
}

void ServicesPage::slotSelectionChanged()
{
    updateButtons();
    loadDetails();
}

void ServicesPage::slotDetailTabChanged()
{
    m_detailsFor = TQString::null;
    loadDetails();
}

void ServicesPage::loadDetails()
{
    TQString unit = currentUnit();
    TQWidget *tab = m_details->currentPage();
    TQString key = unit + "|" + TQString::number((long)tab);
    if (key == m_detailsFor)
        return;
    m_detailsFor = key;

    TQTextEdit *view = static_cast<TQTextEdit *>(tab);
    if (unit.isEmpty()) {
        view->setText(i18n("Select a unit to see its details."));
        return;
    }

    SdResult r;
    if (tab == m_status) {
        r = SdProcess::tool("systemctl", scope(), TQStringList("status") << "--no-pager" << "--full" << "-n" << "0" << "--" << unit);
    } else if (tab == m_file) {
        r = SdProcess::tool("systemctl", scope(), TQStringList("cat") << "--no-pager" << "--" << unit);
    } else if (tab == m_props) {
        r = SdProcess::tool("systemctl", scope(), TQStringList("show") << "--no-pager" << "--" << unit);
    } else if (tab == m_deps) {
        r = SdProcess::tool("systemctl", scope(), TQStringList("list-dependencies") << "--plain" << "--no-pager" << "--" << unit);
    } else if (tab == m_log) {
        TQStringList args;
        args << "--no-pager" << "-n" << "200" << "-o" << "short-iso";
        args << (scope() == UserScope ? "--user-unit" : "--unit") << unit;
        r = SdProcess::tool("journalctl", scope(), args);
    }
    TQString text = r.output();
    // "systemctl status" returns 3 for inactive units; show whatever we got
    if (text.stripWhiteSpace().isEmpty())
        text = r.errorText();
    view->setText(text);
    if (tab == m_log)
        view->scrollToBottom();
}

void ServicesPage::slotContextMenu(TDEListView *, TQListViewItem *item, const TQPoint &pos)
{
    if (!item)
        return;
    TDEPopupMenu menu(this);
    menu.insertTitle(item->text(ColUnit));
    menu.insertItem(SmallIconSet("media-playback-start"), i18n("Start"), ActStart);
    menu.insertItem(SmallIconSet("media-playback-stop"), i18n("Stop"), ActStop);
    menu.insertItem(SmallIconSet("reload"), i18n("Restart"), ActRestart);
    menu.insertItem(SmallIconSet("reload_page"), i18n("Reload"), ActReload);
    menu.insertSeparator();
    menu.insertItem(SmallIconSet("button_ok"), i18n("Enable"), ActEnable);
    menu.insertItem(SmallIconSet("button_cancel"), i18n("Disable"), ActDisable);
    menu.insertItem(SmallIconSet("system-lock-screen"), i18n("Mask"), ActMask);
    menu.insertItem(SmallIconSet("unlock"), i18n("Unmask"), ActUnmask);
    menu.insertSeparator();
    menu.insertItem(SmallIconSet("edit-undo"), i18n("Reset Failed State"), ActResetFailed);
    menu.insertItem(SmallIconSet("process-stop"), i18n("Kill"), ActKill);
    menu.setItemEnabled(ActStart, m_start->isEnabled());
    menu.setItemEnabled(ActStop, m_stop->isEnabled());
    menu.setItemEnabled(ActRestart, m_restart->isEnabled());
    menu.setItemEnabled(ActReload, m_reload->isEnabled());
    menu.setItemEnabled(ActEnable, m_enable->isEnabled());
    menu.setItemEnabled(ActDisable, m_disable->isEnabled());
    menu.setItemEnabled(ActMask, m_mask->isEnabled());
    menu.setItemEnabled(ActUnmask, m_unmask->isEnabled());
    menu.setItemEnabled(ActResetFailed, item->text(ColActive) == "failed");
    menu.setItemEnabled(ActKill, m_stop->isEnabled());
    int id = menu.exec(pos);
    if (id > 0)
        slotAction(id);
}

void ServicesPage::slotAction(int id)
{
    switch (id) {
    case ActStart: runAction("start"); break;
    case ActStop: runAction("stop"); break;
    case ActRestart: runAction("restart"); break;
    case ActReload: runAction("reload"); break;
    case ActEnable: runAction("enable"); break;
    case ActDisable: runAction("disable"); break;
    case ActMask: runAction("mask"); break;
    case ActUnmask: runAction("unmask"); break;
    case ActResetFailed: runAction("reset-failed"); break;
    case ActKill: runAction("kill"); break;
    default: break;
    }
}

void ServicesPage::runAction(const TQString &verb)
{
    TQString unit = currentUnit();
    if (unit.isEmpty())
        return;
    if (verb == "mask" || verb == "kill" ||
        (verb == "stop" && scope() == SystemScope &&
         (unit == "dbus.service" || unit == "systemd-logind.service" || unit.startsWith("display-manager") ||
          unit == "tdm.service"))) {
        if (KMessageBox::warningContinueCancel(
                this, i18n("Do you really want to %1 the unit <b>%2</b>?").arg(SdUnits::verbText(verb)).arg(unit),
                i18n("Confirm"), KGuiItem(i18n("Continue"))) != KMessageBox::Continue)
            return;
    }
    SdUnits::action(this, scope(), verb, unit);
    refresh();
}

void ServicesPage::slotStart() { runAction("start"); }
void ServicesPage::slotStop() { runAction("stop"); }
void ServicesPage::slotRestart() { runAction("restart"); }
void ServicesPage::slotReload() { runAction("reload"); }
void ServicesPage::slotEnable() { runAction("enable"); }
void ServicesPage::slotDisable() { runAction("disable"); }
void ServicesPage::slotMask() { runAction("mask"); }
void ServicesPage::slotUnmask() { runAction("unmask"); }

void ServicesPage::slotDaemonReload()
{
    if (SdUnits::managerAction(this, scope(), "daemon-reload"))
        refresh();
}

#include "servicespage.moc"
