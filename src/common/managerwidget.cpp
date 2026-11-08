/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "managerwidget.h"

#include "datetimepage.h"
#include "hostnamepage.h"
#include "localepage.h"
#include "overviewpage.h"
#include "servicespage.h"
#include "timerspage.h"

#include <tqlayout.h>
#include <tqtabwidget.h>
#include <tqtimer.h>

#include <kiconloader.h>
#include <tdeconfig.h>
#include <tdelocale.h>

static const char *const s_pageKeys[] = { "ShowDateTime", "ShowHostname", "ShowLocale" };

ManagerWidget::ManagerWidget(TQWidget *parent, const char *name)
    : TQWidget(parent, name)
{
    TQVBoxLayout *l = new TQVBoxLayout(this, 0, 0);
    m_tabs = new TQTabWidget(this);
    m_tabs->setMargin(6);
    l->addWidget(m_tabs);
    connect(m_tabs, TQ_SIGNAL(currentChanged(TQWidget *)), this, TQ_SLOT(slotCurrentChanged(TQWidget *)));
    rebuild();
}

bool ManagerWidget::pageEnabled(OptionalPage page)
{
    TDEConfig cfg("tde-systemdrc", true);
    cfg.setGroup("Pages");
    return cfg.readBoolEntry(s_pageKeys[page], true);
}

void ManagerWidget::setPageEnabled(OptionalPage page, bool on)
{
    TDEConfig cfg("tde-systemdrc");
    cfg.setGroup("Pages");
    cfg.writeEntry(s_pageKeys[page], on);
    cfg.sync();
}

bool ManagerWidget::pageAvailable(OptionalPage page)
{
    switch (page) {
    case DateTime: return DateTimePage::isAvailable();
    case Hostname: return HostnamePage::isAvailable();
    case Locale: return LocalePage::isAvailable();
    }
    return false;
}

void ManagerWidget::rebuild()
{
    TQString current;
    if (m_tabs->currentPage())
        current = m_tabs->currentPage()->name();

    m_tabs->blockSignals(true);
    while (m_tabs->count() > 0) {
        TQWidget *w = m_tabs->page(0);
        m_tabs->removePage(w);
        delete w;
    }
    m_pages.clear();

    m_pages.append(new OverviewPage(m_tabs, "overview"));
    m_pages.append(new ServicesPage(m_tabs, "services"));
    m_pages.append(new TimersPage(m_tabs, "timers"));
    if (pageEnabled(DateTime) && pageAvailable(DateTime))
        m_pages.append(new DateTimePage(m_tabs, "datetime"));
    if (pageEnabled(Hostname) && pageAvailable(Hostname))
        m_pages.append(new HostnamePage(m_tabs, "hostname"));
    if (pageEnabled(Locale) && pageAvailable(Locale))
        m_pages.append(new LocalePage(m_tabs, "locale"));

    for (SdPage *p = m_pages.first(); p; p = m_pages.next())
        m_tabs->addTab(p, SmallIconSet(p->iconName()), p->title());
    m_tabs->blockSignals(false);

    if (!current.isEmpty())
        showPage(current);
    // load the visible page once the window is shown
    TQTimer::singleShot(0, this, TQ_SLOT(refreshCurrent()));
}

void ManagerWidget::showPage(const TQString &name)
{
    for (SdPage *p = m_pages.first(); p; p = m_pages.next()) {
        if (name == p->name())
            m_tabs->showPage(p);
    }
}

void ManagerWidget::selectUnit(const TQString &unit, bool user)
{
    for (SdPage *p = m_pages.first(); p; p = m_pages.next()) {
        ServicesPage *sp = dynamic_cast<ServicesPage *>(p);
        if (sp) {
            m_tabs->blockSignals(true);
            m_tabs->showPage(sp);
            m_tabs->blockSignals(false);
            sp->selectUnit(unit, user ? UserScope : SystemScope);
        }
    }
}

void ManagerWidget::slotCurrentChanged(TQWidget *w)
{
    SdPage *p = dynamic_cast<SdPage *>(w);
    if (p)
        p->ensureLoaded();
}

void ManagerWidget::refreshCurrent()
{
    SdPage *p = dynamic_cast<SdPage *>(m_tabs->currentPage());
    if (!p)
        return;
    if (p->isLoaded())
        p->refresh();
    else
        p->ensureLoaded();
}

#include "managerwidget.moc"
