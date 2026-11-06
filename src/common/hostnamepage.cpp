/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "hostnamepage.h"
#include "sdprocess.h"

#include <tqcombobox.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqregexp.h>
#include <tqstylesheet.h>
#include <tqvalidator.h>

#include <kiconloader.h>
#include <klineedit.h>
#include <tdelocale.h>
#include <kpushbutton.h>

static const char *const s_chassis[] = {
    "", "desktop", "laptop", "convertible", "server", "tablet", "handset", "watch", "embedded", "vm", "container", 0
};

HostnamePage::HostnamePage(TQWidget *parent, const char *name)
    : SdPage(parent, name)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 8);

    TQGroupBox *names = new TQGroupBox(i18n("Host Names"), this);
    names->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *g = new TQGridLayout(names->layout(), 6, 2, 6);
    int row = 0;
    m_static = new KLineEdit(names);
    g->addWidget(new TQLabel(m_static, i18n("&Static hostname:"), names), row, 0);
    m_static->setValidator(new TQRegExpValidator(TQRegExp("[A-Za-z0-9.-]{0,64}"), m_static));
    g->addWidget(m_static, row++, 1);
    m_pretty = new KLineEdit(names);
    g->addWidget(new TQLabel(m_pretty, i18n("&Pretty hostname:"), names), row, 0);
    g->addWidget(m_pretty, row++, 1);
    m_icon = new KLineEdit(names);
    g->addWidget(new TQLabel(m_icon, i18n("&Icon name:"), names), row, 0);
    g->addWidget(m_icon, row++, 1);
    m_chassis = new TQComboBox(names);
    g->addWidget(new TQLabel(m_chassis, i18n("&Chassis:"), names), row, 0);
    m_chassis->insertItem(i18n("(automatic)"));
    for (int i = 1; s_chassis[i]; ++i)
        m_chassis->insertItem(s_chassis[i]);
    g->addWidget(m_chassis, row++, 1, TQt::AlignLeft);
    m_deployment = new KLineEdit(names);
    g->addWidget(new TQLabel(m_deployment, i18n("&Deployment:"), names), row, 0);
    m_deployment->setClickMessage(i18n("e.g. production, staging, development"));
    g->addWidget(m_deployment, row++, 1);
    m_location = new KLineEdit(names);
    g->addWidget(new TQLabel(m_location, i18n("&Location:"), names), row, 0);
    g->addWidget(m_location, row++, 1);
    g->setColStretch(1, 1);
    top->addWidget(names);

    TQGroupBox *infoBox = new TQGroupBox(i18n("Machine Information"), this);
    infoBox->setColumnLayout(0, TQt::Vertical);
    TQVBoxLayout *il = new TQVBoxLayout(infoBox->layout());
    m_info = new TQLabel(infoBox);
    m_info->setTextFormat(TQt::RichText);
    il->addWidget(m_info);
    top->addWidget(infoBox);

    TQHBoxLayout *buttons = new TQHBoxLayout(top, 6);
    buttons->addStretch(1);
    KPushButton *refreshBtn = new KPushButton(SmallIconSet("reload"), i18n("&Reset"), this);
    m_apply = new KPushButton(SmallIconSet("apply"), i18n("&Apply"), this);
    buttons->addWidget(refreshBtn);
    buttons->addWidget(m_apply);
    top->addStretch(1);

    const char *sig = TQ_SIGNAL(textChanged(const TQString &));
    connect(m_static, sig, this, TQ_SLOT(slotChanged()));
    connect(m_pretty, sig, this, TQ_SLOT(slotChanged()));
    connect(m_icon, sig, this, TQ_SLOT(slotChanged()));
    connect(m_deployment, sig, this, TQ_SLOT(slotChanged()));
    connect(m_location, sig, this, TQ_SLOT(slotChanged()));
    connect(m_chassis, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotChanged()));
    connect(refreshBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(refresh()));
    connect(m_apply, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotApply()));
    m_apply->setEnabled(false);
}

TQString HostnamePage::title() const { return i18n("Hostname"); }
TQString HostnamePage::iconName() const { return "network"; }

bool HostnamePage::isAvailable()
{
    return !SdProcess::findTool("hostnamectl").isNull() && SdProcess::hasSystemdService("hostname1");
}

TQString HostnamePage::current(const TQString &what) const
{
    return m_cur.contains(what) ? m_cur[what] : TQString::null;
}

void HostnamePage::doRefresh()
{
    m_cur.clear();
    SdResult r = SdProcess::tool("hostnamectl", SystemScope, TQStringList("--static") << "hostname");
    m_cur["static"] = r.output().stripWhiteSpace();
    r = SdProcess::tool("hostnamectl", SystemScope, TQStringList("--pretty") << "hostname");
    m_cur["pretty"] = r.output().stripWhiteSpace();
    m_cur["icon"] = SdProcess::tool("hostnamectl", SystemScope, TQStringList("icon-name")).output().stripWhiteSpace();
    m_cur["chassis"] = SdProcess::tool("hostnamectl", SystemScope, TQStringList("chassis")).output().stripWhiteSpace();
    m_cur["deployment"] = SdProcess::tool("hostnamectl", SystemScope, TQStringList("deployment")).output().stripWhiteSpace();
    m_cur["location"] = SdProcess::tool("hostnamectl", SystemScope, TQStringList("location")).output().stripWhiteSpace();

    m_static->setText(m_cur["static"]);
    m_pretty->setText(m_cur["pretty"]);
    m_icon->setText(m_cur["icon"]);
    m_deployment->setText(m_cur["deployment"]);
    m_location->setText(m_cur["location"]);
    m_chassis->setCurrentItem(0);
    for (int i = 1; s_chassis[i]; ++i) {
        if (m_cur["chassis"] == s_chassis[i])
            m_chassis->setCurrentItem(i);
    }
    // with an automatic chassis hostnamectl reports the detected value; treat it as current
    m_cur["chassisIndex"] = TQString::number(m_chassis->currentItem());

    r = SdProcess::tool("hostnamectl", SystemScope, TQStringList("status"));
    TQString html = "<table cellspacing=\"2\">";
    TQStringList lines = r.lines();
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        int p = (*it).find(':');
        if (p <= 0)
            continue;
        TQString key = (*it).left(p).stripWhiteSpace();
        if (key.endsWith("hostname") || key == "Icon name" || key == "Chassis" ||
            key == "Deployment" || key == "Location")
            continue;
        html += TQString("<tr><td align=\"right\"><b>%1:</b></td><td>%2</td></tr>")
                    .arg(TQStyleSheet::escape(key)).arg(TQStyleSheet::escape(sdStripSymbols((*it).mid(p + 1))));
    }
    html += "</table>";
    m_info->setText(r.ok() ? html : TQStyleSheet::escape(r.errorText()));
    m_apply->setEnabled(false);
}

void HostnamePage::slotChanged()
{
    bool changed = m_static->text() != m_cur["static"] || m_pretty->text() != m_cur["pretty"] ||
                   m_icon->text() != m_cur["icon"] || m_deployment->text() != m_cur["deployment"] ||
                   m_location->text() != m_cur["location"] ||
                   TQString::number(m_chassis->currentItem()) != m_cur["chassisIndex"];
    m_apply->setEnabled(changed && !m_static->text().isEmpty());
}

void HostnamePage::slotApply()
{
    struct Change { const char *key; const char *verb; TQString value; };
    Change changes[] = {
        { "static", "set-hostname", m_static->text().stripWhiteSpace() },
        { "pretty", "set-pretty", m_pretty->text() },
        { "icon", "set-icon", m_icon->text().stripWhiteSpace() },
        { "deployment", "set-deployment", m_deployment->text().stripWhiteSpace() },
        { "location", "set-location", m_location->text() },
    };
    for (unsigned i = 0; i < sizeof(changes) / sizeof(changes[0]); ++i) {
        if (changes[i].value == m_cur[changes[i].key])
            continue;
        SdResult r = SdProcess::privileged("hostname", TQStringList(changes[i].verb) << changes[i].value);
        if (!r.ok()) {
            SdProcess::showError(this, i18n("Could not apply the new value for \"%1\".").arg(changes[i].key), r);
            break;
        }
    }
    if (TQString::number(m_chassis->currentItem()) != m_cur["chassisIndex"]) {
        SdResult r = SdProcess::privileged("hostname", TQStringList("set-chassis") << s_chassis[m_chassis->currentItem()]);
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not change the chassis type."), r);
    }
    refresh();
}

#include "hostnamepage.moc"
