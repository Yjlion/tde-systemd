/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "journalwidget.h"
#include "maintenancedialog.h"
#include "sdjson.h"

#include <config.h>

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqdatetime.h>
#include <tqfile.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqscrollbar.h>
#include <tqspinbox.h>
#include <tqsplitter.h>
#include <tqstylesheet.h>
#include <tqtextedit.h>
#include <tqtextstream.h>
#include <tqtimer.h>
#include <tqtooltip.h>

#include <kiconloader.h>
#include <klineedit.h>
#include <tdefiledialog.h>
#include <tdeglobalsettings.h>
#include <tdelistview.h>
#include <tdelistviewsearchline.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kpushbutton.h>

namespace {

enum Columns { ColTime = 0, ColPriority, ColSource, ColPid, ColMessage };

const char *const s_priorities[] = {
    I18N_NOOP("Emergency"), I18N_NOOP("Alert"), I18N_NOOP("Critical"), I18N_NOOP("Error"),
    I18N_NOOP("Warning"), I18N_NOOP("Notice"), I18N_NOOP("Info"), I18N_NOOP("Debug")
};

class EntryItem : public TDEListViewItem
{
public:
    EntryItem(TDEListView *parent, TQListViewItem *after, const SdJson &e)
        : TDEListViewItem(parent, after), entry(e)
    {
        bool ok;
        priority = e.str("PRIORITY").toInt(&ok);
        if (!ok)
            priority = 6;

        unsigned long long usec = e["__REALTIME_TIMESTAMP"].toULongLong();
        TQDateTime dt;
        dt.setTime_t((uint)(usec / 1000000ULL));
        setText(ColTime, dt.toString("yyyy-MM-dd hh:mm:ss") +
                TQString(".%1").arg((uint)((usec / 1000ULL) % 1000ULL), 3, 10).replace(' ', '0'));
        setText(ColPriority, i18n(s_priorities[priority & 7]));
        TQString source = e.str("SYSLOG_IDENTIFIER");
        if (source.isEmpty())
            source = e.str("_COMM");
        if (source.isEmpty() && e.str("_TRANSPORT") == "kernel")
            source = "kernel";
        setText(ColSource, source);
        setText(ColPid, e.str("_PID"));
        TQString msg = e.str("MESSAGE");
        int nl = msg.find('\n');
        setText(ColMessage, nl >= 0 ? msg.left(nl) + TQString::fromUtf8(" \xe2\x80\xa6") : msg);

        if (priority <= 3)
            setPixmap(ColPriority, SmallIcon("messagebox_critical"));
        else if (priority == 4)
            setPixmap(ColPriority, SmallIcon("messagebox_warning"));
        else if (priority == 5)
            setPixmap(ColPriority, SmallIcon("messagebox_info"));
    }

    void paintCell(TQPainter *p, const TQColorGroup &cg, int column, int width, int align)
    {
        TQColorGroup g(cg);
        if (priority <= 3)
            g.setColor(TQColorGroup::Text, TQColor(0xc0, 0x00, 0x00));
        else if (priority == 4)
            g.setColor(TQColorGroup::Text, TQColor(0xb0, 0x60, 0x00));
        else if (priority == 7)
            g.setColor(TQColorGroup::Text, TQt::darkGray);
        if (priority <= 5 && column == ColMessage) {
            TQFont f = p->font();
            f.setBold(true);
            p->setFont(f);
        }
        TDEListViewItem::paintCell(p, g, column, width, align);
    }

    SdJson entry;
    int priority;
};

} // namespace

JournalWidget::JournalWidget(TQWidget *parent, const char *name)
    : TQWidget(parent, name), m_following(false), m_loadingThenFollow(false), m_count(0)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 6, 6);

    // ---- filters
    TQGroupBox *filters = new TQGroupBox(i18n("Filter"), this);
    filters->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *g = new TQGridLayout(filters->layout(), 3, 8, 6);

    g->addWidget(new TQLabel(i18n("Journal:"), filters), 0, 0);
    m_scope = new TQComboBox(filters);
    m_scope->insertItem(SmallIcon("computer"), i18n("System"));
    m_scope->insertItem(SmallIcon("go-home"), i18n("User"));
    g->addWidget(m_scope, 0, 1);

    g->addWidget(new TQLabel(i18n("Boot:"), filters), 0, 2);
    m_boot = new TQComboBox(filters);
    g->addWidget(m_boot, 0, 3);

    g->addWidget(new TQLabel(i18n("Priority:"), filters), 0, 4);
    m_priority = new TQComboBox(filters);
    for (int i = 0; i < 8; ++i)
        m_priority->insertItem(i18n("%1 and higher").arg(i18n(s_priorities[i])));
    m_priority->setCurrentItem(7);
    g->addWidget(m_priority, 0, 5);

    g->addWidget(new TQLabel(i18n("Lines:"), filters), 0, 6);
    m_lines = new TQSpinBox(100, 1000000, 100, filters);
    m_lines->setValue(1000);
    g->addWidget(m_lines, 0, 7);

    g->addWidget(new TQLabel(i18n("Unit:"), filters), 1, 0);
    m_unit = new TQComboBox(true, filters);
    m_unit->setInsertionPolicy(TQComboBox::NoInsertion);
    g->addWidget(m_unit, 1, 1);

    g->addWidget(new TQLabel(i18n("Identifier:"), filters), 1, 2);
    m_ident = new KLineEdit(filters);
    TQToolTip::add(m_ident, i18n("Syslog identifier, e.g. sshd or kernel"));
    g->addWidget(m_ident, 1, 3);

    g->addWidget(new TQLabel(i18n("Since:"), filters), 1, 4);
    m_since = new KLineEdit(filters);
    TQToolTip::add(m_since, i18n("e.g. \"today\", \"yesterday\", \"1 hour ago\" or \"2026-11-06 10:00\""));
    g->addWidget(m_since, 1, 5);

    g->addWidget(new TQLabel(i18n("Until:"), filters), 1, 6);
    m_until = new KLineEdit(filters);
    TQToolTip::add(m_until, i18n("e.g. \"now\", \"10 min ago\" or \"2026-11-06 12:00\""));
    g->addWidget(m_until, 1, 7);

    g->addWidget(new TQLabel(i18n("Message:"), filters), 2, 0);
    m_grep = new KLineEdit(filters);
    TQToolTip::add(m_grep, i18n("Only show entries whose message matches this regular expression"));
    g->addWidget(m_grep, 2, 1);

    m_kernel = new TQCheckBox(i18n("&Kernel messages only"), filters);
    g->addMultiCellWidget(m_kernel, 2, 2, 2, 3);
    m_admin = new TQCheckBox(i18n("Read &all as administrator"), filters);
    TQToolTip::add(m_admin, i18n("Read the complete system journal with administrator rights. "
                                "Members of the systemd-journal group can read it without this."));
    m_admin->setEnabled(SdProcess::privilegedAvailable());
    g->addMultiCellWidget(m_admin, 2, 2, 4, 5);

    TQHBoxLayout *fb = new TQHBoxLayout(6);
    KPushButton *clear = new KPushButton(SmallIconSet("edit-clear"), i18n("C&lear"), filters);
    KPushButton *apply = new KPushButton(SmallIconSet("reload"), i18n("&Refresh"), filters);
    fb->addStretch(1);
    fb->addWidget(clear);
    fb->addWidget(apply);
    g->addMultiCellLayout(fb, 2, 2, 6, 7);
    for (int c = 1; c < 8; c += 2)
        g->setColStretch(c, 1);
    top->addWidget(filters);

    // ---- search / follow bar
    TQHBoxLayout *bar = new TQHBoxLayout(top, 6);
    bar->addWidget(new TQLabel(i18n("Search in results:"), this));
    m_search = new TDEListViewSearchLine(this);
    bar->addWidget(m_search, 1);
    m_follow = new KPushButton(SmallIconSet("media-playback-start"), i18n("&Follow"), this);
    m_follow->setToggleButton(true);
    TQToolTip::add(m_follow, i18n("Show new entries as they arrive"));
    bar->addWidget(m_follow);
    KPushButton *maint = new KPushButton(SmallIconSet("configure"), i18n("&Maintenance..."), this);
    bar->addWidget(maint);

    // ---- list and detail
    TQSplitter *split = new TQSplitter(TQt::Vertical, this);
    top->addWidget(split, 1);
    m_list = new TDEListView(split);
    m_list->addColumn(i18n("Time"));
    m_list->addColumn(i18n("Priority"));
    m_list->addColumn(i18n("Source"));
    m_list->addColumn(i18n("PID"));
    m_list->addColumn(i18n("Message"));
    m_list->setSorting(-1);
    m_list->setAllColumnsShowFocus(true);
    m_list->setColumnWidthMode(ColMessage, TQListView::Maximum);
    m_list->setColumnAlignment(ColPid, TQt::AlignRight);
    m_search->setListView(m_list);

    m_detail = new TQTextEdit(split);
    m_detail->setReadOnly(true);
    m_detail->setTextFormat(TQt::RichText);
    TQValueList<int> sizes;
    sizes << 360 << 140;
    split->setSizes(sizes);

    m_status = new TQLabel(this);
    top->addWidget(m_status);

    m_reader = new SdLineReader(this);
    m_flushTimer = new TQTimer(this);

    connect(m_scope, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotScopeChanged()));
    connect(m_boot, TQ_SIGNAL(activated(int)), this, TQ_SLOT(reload()));
    connect(m_priority, TQ_SIGNAL(activated(int)), this, TQ_SLOT(reload()));
    connect(m_unit, TQ_SIGNAL(activated(int)), this, TQ_SLOT(reload()));
    connect(m_kernel, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(reload()));
    connect(m_admin, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(reload()));
    connect(m_ident, TQ_SIGNAL(returnPressed()), this, TQ_SLOT(reload()));
    connect(m_since, TQ_SIGNAL(returnPressed()), this, TQ_SLOT(reload()));
    connect(m_until, TQ_SIGNAL(returnPressed()), this, TQ_SLOT(reload()));
    connect(m_grep, TQ_SIGNAL(returnPressed()), this, TQ_SLOT(reload()));
    connect(clear, TQ_SIGNAL(clicked()), this, TQ_SLOT(clearFilters()));
    connect(apply, TQ_SIGNAL(clicked()), this, TQ_SLOT(reload()));
    connect(m_follow, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(setFollow(bool)));
    connect(maint, TQ_SIGNAL(clicked()), this, TQ_SLOT(openMaintenance()));
    connect(m_list, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(slotSelectionChanged()));
    connect(m_reader, TQ_SIGNAL(lineReceived(const TQCString &)), this, TQ_SLOT(slotLine(const TQCString &)));
    connect(m_reader, TQ_SIGNAL(finished(int, const TQString &)), this, TQ_SLOT(slotFinished(int, const TQString &)));
    connect(m_flushTimer, TQ_SIGNAL(timeout()), this, TQ_SLOT(slotFlush()));

    loadBoots();
    loadUnits();
    TQTimer::singleShot(0, this, TQ_SLOT(reload()));
}

JournalWidget::~JournalWidget()
{
    m_reader->stop();
}

SdScope JournalWidget::scope() const
{
    return m_scope->currentItem() == 1 ? UserScope : SystemScope;
}

bool JournalWidget::isFollowing() const
{
    return m_following;
}

void JournalWidget::setScope(SdScope s)
{
    m_scope->setCurrentItem(s == UserScope ? 1 : 0);
    slotScopeChanged();
}

void JournalWidget::setUnitFilter(const TQString &unit)
{
    m_unit->setEditText(unit);
}

void JournalWidget::setPriorityFilter(int priority)
{
    if (priority >= 0 && priority < 8)
        m_priority->setCurrentItem(priority);
}

void JournalWidget::setBootFilter(const TQString &offset)
{
    for (int i = 0; i < m_boot->count(); ++i) {
        if (m_boot->text(i).startsWith(offset + ":") || (offset == "all" && i == m_boot->count() - 1))
            m_boot->setCurrentItem(i);
    }
}

void JournalWidget::clearFilters()
{
    m_boot->setCurrentItem(0);
    m_priority->setCurrentItem(7);
    m_unit->setEditText(TQString::null);
    m_ident->clear();
    m_since->clear();
    m_until->clear();
    m_grep->clear();
    m_kernel->setChecked(false);
    m_search->clear();
    reload();
}

void JournalWidget::slotScopeChanged()
{
    m_admin->setEnabled(scope() == SystemScope && SdProcess::privilegedAvailable());
    if (scope() == UserScope)
        m_admin->setChecked(false);
    m_kernel->setEnabled(scope() == SystemScope);
    loadBoots();
    loadUnits();
    reload();
}

void JournalWidget::loadBoots()
{
    m_boot->clear();
    m_boot->insertItem(i18n("Current boot"));
    SdResult r = SdProcess::tool("journalctl", scope(), TQStringList("--list-boots") << "--no-pager");
    TQStringList lines = r.lines();
    // newest first; skip the header and the current boot (offset 0)
    for (int i = (int)lines.count() - 1; i >= 0; --i) {
        TQStringList f = TQStringList::split(' ', lines[i].simplifyWhiteSpace());
        if (f.count() < 3 || f[0] == "IDX" || f[0] == "0")
            continue;
        TQString range = lines[i].simplifyWhiteSpace().section(' ', 2);
        m_boot->insertItem(TQString("%1: %2").arg(f[0]).arg(range));
    }
    m_boot->insertItem(i18n("All boots"));
}

void JournalWidget::loadUnits()
{
    TQString keep = m_unit->currentText();
    m_unit->clear();
    m_unit->insertItem(TQString::null);
    TQString field = scope() == UserScope ? "_SYSTEMD_USER_UNIT" : "_SYSTEMD_UNIT";
    SdResult r = SdProcess::tool("journalctl", scope(), TQStringList("-F") << field);
    TQStringList units = r.lines();
    units.sort();
    m_unit->insertStringList(units);
    m_unit->setEditText(keep);
}

TQStringList JournalWidget::filterArgs(bool forHelper) const
{
    TQStringList a;
    int b = m_boot->currentItem();
    if (b == 0)
        a << "-b" << "0";
    else if (b < m_boot->count() - 1)
        a << "-b" << m_boot->currentText().section(':', 0, 0);
    if (m_priority->currentItem() < 7)
        a << "-p" << TQString::number(m_priority->currentItem());
    TQString unit = m_unit->currentText().stripWhiteSpace();
    if (!unit.isEmpty())
        a << (scope() == UserScope && !forHelper ? "--user-unit" : "-u") << unit;
    if (!m_ident->text().stripWhiteSpace().isEmpty())
        a << "-t" << m_ident->text().stripWhiteSpace();
    if (!m_since->text().stripWhiteSpace().isEmpty())
        a << "-S" << m_since->text().stripWhiteSpace();
    if (!m_until->text().stripWhiteSpace().isEmpty())
        a << "-U" << m_until->text().stripWhiteSpace();
    if (!m_grep->text().isEmpty())
        a << "-g" << m_grep->text();
    if (m_kernel->isChecked() && scope() == SystemScope)
        a << "-k";
    return a;
}

void JournalWidget::startReader(bool follow)
{
    TQStringList argv;
    bool admin = m_admin->isChecked() && m_admin->isEnabled();
    if (admin && !follow) {
        argv << "pkexec" << "--disable-internal-agent" << TDESYSTEMD_HELPER << "journal-read";
        argv += filterArgs(true);
        argv << "-n" << TQString::number(m_lines->value());
    } else {
        argv << "journalctl";
        if (scope() == UserScope)
            argv << "--user";
        argv << "--no-pager" << "-o" << "json";
        argv += filterArgs(false);
        if (follow) {
            argv << "-f";
            if (!m_lastCursor.isEmpty())
                argv << ("--after-cursor=" + m_lastCursor);
            else
                argv << "-n" << "0";
        } else {
            argv << "-n" << TQString::number(m_lines->value());
        }
    }
    if (!m_reader->start(argv))
        m_status->setText(i18n("Could not start journalctl."));
    m_flushTimer->start(200);
}

void JournalWidget::reload()
{
    m_reader->stop();
    m_pending.clear();
    m_list->clear();
    m_detail->clear();
    m_lastCursor = TQString::null;
    m_count = 0;
    m_status->setText(i18n("Loading..."));
    m_loadingThenFollow = m_following;
    startReader(false);
}

void JournalWidget::setFollow(bool on)
{
    if (on && m_admin->isChecked()) {
        KMessageBox::information(this, i18n("Following new entries is not possible while reading the journal "
                                            "as administrator. The journal is followed with your own permissions."),
                                 TQString::null, "journalFollowAdmin");
    }
    m_following = on;
    m_follow->blockSignals(true);
    m_follow->setOn(on);
    m_follow->blockSignals(false);
    m_follow->setIconSet(SmallIconSet(on ? "media-playback-pause" : "media-playback-start"));
    if (on) {
        if (!m_reader->isRunning())
            startReader(true);
        else
            m_loadingThenFollow = true;
    } else {
        m_loadingThenFollow = false;
        m_reader->stop();
        slotFlush();
        m_flushTimer->stop();
        m_status->setText(i18n("%n entry", "%n entries", m_count));
    }
    emit followChanged(on);
}

void JournalWidget::toggleFollow()
{
    setFollow(!m_following);
}

void JournalWidget::slotLine(const TQCString &line)
{
    m_pending.append(line);
}

void JournalWidget::slotFlush()
{
    if (m_pending.isEmpty())
        return;
    TQScrollBar *sb = m_list->verticalScrollBar();
    bool atEnd = sb->value() >= sb->maxValue() - 4;

    m_list->setUpdatesEnabled(false);
    TQListViewItem *last = m_list->lastItem();
    for (TQValueList<TQCString>::ConstIterator it = m_pending.begin(); it != m_pending.end(); ++it) {
        bool ok = false;
        SdJson e = SdJson::parse(*it, &ok);
        if (!ok || !e.isObject())
            continue;
        last = new EntryItem(m_list, last, e);
        m_lastCursor = e.str("__CURSOR");
        ++m_count;
    }
    m_pending.clear();

    // keep the list bounded while following
    int limit = m_lines->value() * 2;
    while (m_following && m_list->childCount() > limit) {
        delete m_list->firstChild();
        --m_count;
    }
    m_list->setUpdatesEnabled(true);
    m_list->triggerUpdate();
    m_search->updateSearch();
    if (atEnd || m_following)
        m_list->ensureItemVisible(m_list->lastItem());
    m_status->setText(m_following ? i18n("%n entry, following...", "%n entries, following...", m_count)
                                  : i18n("%n entry", "%n entries", m_count));
}

void JournalWidget::slotFinished(int exitCode, const TQString &error)
{
    slotFlush();
    if (exitCode != 0 && m_count == 0) {
        TQString msg = error.isEmpty() ? i18n("journalctl failed with exit code %1.").arg(exitCode) : error;
        if (exitCode == 126 || exitCode == 127)
            msg = i18n("Authorization failed or was cancelled.");
        m_status->setText(msg);
    } else {
        m_status->setText(i18n("%n entry", "%n entries", m_count));
        if (m_count == 0)
            m_status->setText(i18n("No entries match the filter."));
        if (!error.isEmpty() && error.contains("not seeing messages from other users"))
            m_status->setText(m_status->text() + "  " +
                              i18n("(Only your own messages are shown. Use \"Read all as administrator\" or join the systemd-journal group.)"));
    }
    if (m_loadingThenFollow) {
        m_loadingThenFollow = false;
        startReader(true);
    } else {
        m_flushTimer->stop();
    }
    if (m_list->lastItem()) {
        if (!m_list->selectedItem())
            m_list->setSelected(m_list->lastItem(), true);
        m_list->ensureItemVisible(m_list->lastItem());
    }
}

void JournalWidget::slotSelectionChanged()
{
    EntryItem *item = static_cast<EntryItem *>(m_list->selectedItem());
    if (!item) {
        m_detail->clear();
        return;
    }
    const SdJson &e = item->entry;
    TQString html = "<p><b>" + TQStyleSheet::escape(item->text(ColTime)) + "</b> &nbsp; " +
                    TQStyleSheet::escape(item->text(ColPriority)) + " &nbsp; " +
                    TQStyleSheet::escape(item->text(ColSource)) + "</p>";
    html += "<pre>" + TQStyleSheet::escape(e.str("MESSAGE")) + "</pre>";
    html += "<table cellspacing=\"0\" cellpadding=\"1\">";
    TQValueList<TQString> keys = e.keys();
    for (TQValueList<TQString>::ConstIterator it = keys.begin(); it != keys.end(); ++it) {
        if (*it == "MESSAGE")
            continue;
        html += "<tr><td><tt><font color=\"#555555\">" + TQStyleSheet::escape(*it) + "</font></tt></td><td><tt>" +
                TQStyleSheet::escape(e.str(*it)) + "</tt></td></tr>";
    }
    html += "</table>";
    m_detail->setText(html);
}

void JournalWidget::exportLog()
{
    TQString file = KFileDialog::getSaveFileName(TQString::null, "*.log *.txt|" + i18n("Log Files") + "\n*|" + i18n("All Files"),
                                                 this, i18n("Export Journal"));
    if (file.isEmpty())
        return;
    TQFile f(file);
    if (!f.open(IO_WriteOnly | IO_Truncate)) {
        KMessageBox::error(this, i18n("Could not write to %1.").arg(file));
        return;
    }
    TQTextStream ts(&f);
    ts.setEncoding(TQTextStream::UnicodeUTF8);
    int written = 0;
    for (TQListViewItem *it = m_list->firstChild(); it; it = it->nextSibling()) {
        if (!it->isVisible())
            continue;
        EntryItem *e = static_cast<EntryItem *>(it);
        TQString host = e->entry.str("_HOSTNAME");
        TQString pid = e->text(ColPid).isEmpty() ? TQString::null : "[" + e->text(ColPid) + "]";
        ts << e->text(ColTime) << " " << host << " " << e->text(ColSource) << pid << " <"
           << e->priority << ">: " << e->entry.str("MESSAGE") << "\n";
        ++written;
    }
    f.close();
    m_status->setText(i18n("Exported %n entry to %1.", "Exported %n entries to %1.", written).arg(file));
}

void JournalWidget::openMaintenance()
{
    MaintenanceDialog dlg(this);
    dlg.exec();
    if (dlg.changedJournal())
        reload();
}

#include "journalwidget.moc"
