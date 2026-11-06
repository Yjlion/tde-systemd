/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "localepage.h"
#include "sdprocess.h"

#include <tqcombobox.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>

#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kpushbutton.h>

static const char *const s_vars[] = {
    "LANG", "LC_CTYPE", "LC_NUMERIC", "LC_TIME", "LC_COLLATE", "LC_MONETARY",
    "LC_MESSAGES", "LC_PAPER", "LC_MEASUREMENT", 0
};

LocalePage::LocalePage(TQWidget *parent, const char *name)
    : SdPage(parent, name), m_listsLoaded(false)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 8);
    TQHBoxLayout *cols = new TQHBoxLayout(top, 8);

    TQGroupBox *loc = new TQGroupBox(i18n("System Locale"), this);
    loc->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *lg = new TQGridLayout(loc->layout(), 10, 2, 6);
    for (int i = 0; s_vars[i]; ++i) {
        TQString label = i == 0 ? i18n("Language (LANG):") : TQString(s_vars[i]) + ":";
        lg->addWidget(new TQLabel(label, loc), i, 0);
        TQComboBox *c = makeCombo(loc, false);
        lg->addWidget(c, i, 1);
        m_vars.insert(s_vars[i], c);
    }
    lg->setColStretch(1, 1);
    cols->addWidget(loc, 1);

    TQGroupBox *kbd = new TQGroupBox(i18n("Keyboard"), this);
    kbd->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *kg = new TQGridLayout(kbd->layout(), 6, 2, 6);
    kg->addWidget(new TQLabel(i18n("Console keymap:"), kbd), 0, 0);
    m_vcKeymap = makeCombo(kbd, true);
    kg->addWidget(m_vcKeymap, 0, 1);
    kg->addWidget(new TQLabel(i18n("X11 layout:"), kbd), 1, 0);
    m_x11Layout = makeCombo(kbd, true);
    kg->addWidget(m_x11Layout, 1, 1);
    kg->addWidget(new TQLabel(i18n("X11 model:"), kbd), 2, 0);
    m_x11Model = makeCombo(kbd, true);
    kg->addWidget(m_x11Model, 2, 1);
    kg->addWidget(new TQLabel(i18n("X11 variant:"), kbd), 3, 0);
    m_x11Variant = makeCombo(kbd, true);
    kg->addWidget(m_x11Variant, 3, 1);
    kg->addWidget(new TQLabel(i18n("X11 options:"), kbd), 4, 0);
    m_x11Options = makeCombo(kbd, true);
    kg->addWidget(m_x11Options, 4, 1);
    TQLabel *note = new TQLabel(i18n("These are the system wide defaults used by the console and the "
                                    "login manager. Your personal TDE keyboard and language settings "
                                    "override them in your session."), kbd);
    note->setAlignment(TQt::WordBreak);
    kg->addMultiCellWidget(note, 5, 5, 0, 1);
    kg->setColStretch(1, 1);
    cols->addWidget(kbd, 1);

    TQHBoxLayout *buttons = new TQHBoxLayout(top, 6);
    buttons->addStretch(1);
    KPushButton *refreshBtn = new KPushButton(SmallIconSet("reload"), i18n("&Reset"), this);
    m_apply = new KPushButton(SmallIconSet("apply"), i18n("&Apply"), this);
    buttons->addWidget(refreshBtn);
    buttons->addWidget(m_apply);
    top->addStretch(1);

    connect(m_x11Layout, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotLayoutChanged()));
    connect(refreshBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(refresh()));
    connect(m_apply, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotApply()));
    m_apply->setEnabled(false);
}

TQComboBox *LocalePage::makeCombo(TQWidget *parent, bool editable)
{
    TQComboBox *c = new TQComboBox(editable, parent);
    c->setInsertionPolicy(TQComboBox::NoInsertion);
    c->setDuplicatesEnabled(false);
    connect(c, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotChanged()));
    if (editable)
        connect(c, TQ_SIGNAL(textChanged(const TQString &)), this, TQ_SLOT(slotChanged()));
    return c;
}

TQString LocalePage::title() const { return i18n("Locale && Keyboard"); }
TQString LocalePage::iconName() const { return "locale"; }

bool LocalePage::isAvailable()
{
    return !SdProcess::findTool("localectl").isNull() && SdProcess::hasSystemdService("locale1");
}

void LocalePage::setComboValue(TQComboBox *c, const TQString &value)
{
    c->blockSignals(true);
    if (c->editable()) {
        c->setEditText(value);
    } else {
        int idx = 0;
        for (int i = 0; i < c->count(); ++i) {
            if (c->text(i) == value)
                idx = i;
        }
        if (idx == 0 && !value.isEmpty()) {
            c->insertItem(value);
            idx = c->count() - 1;
        }
        c->setCurrentItem(idx);
    }
    c->blockSignals(false);
}

TQString LocalePage::comboValue(TQComboBox *c) const
{
    if (c->editable())
        return c->currentText().stripWhiteSpace();
    return c->currentItem() == 0 ? TQString::null : c->currentText();
}

void LocalePage::doRefresh()
{
    if (!m_listsLoaded) {
        m_listsLoaded = true;
        TQStringList locales = SdProcess::tool("localectl", SystemScope, TQStringList("list-locales") << "--no-pager").lines();
        for (TQMap<TQString, TQComboBox *>::Iterator it = m_vars.begin(); it != m_vars.end(); ++it) {
            (*it)->insertItem(it.key() == "LANG" ? i18n("(unset)") : i18n("(same as LANG)"));
            (*it)->insertStringList(locales);
        }
        m_vcKeymap->insertStringList(SdProcess::tool("localectl", SystemScope, TQStringList("list-keymaps") << "--no-pager").lines());
        m_x11Layout->insertStringList(SdProcess::tool("localectl", SystemScope, TQStringList("list-x11-keymap-layouts") << "--no-pager").lines());
        m_x11Model->insertStringList(SdProcess::tool("localectl", SystemScope, TQStringList("list-x11-keymap-models") << "--no-pager").lines());
        m_x11Options->insertStringList(SdProcess::tool("localectl", SystemScope, TQStringList("list-x11-keymap-options") << "--no-pager").lines());
    }

    m_cur.clear();
    SdResult r = SdProcess::tool("localectl", SystemScope, TQStringList("status"));
    TQStringList lines = r.lines();
    TQString section;
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        TQString line = (*it).stripWhiteSpace();
        int colon = line.find(':');
        TQString value = line;
        if (colon > 0 && !line.left(colon).contains('=')) {
            section = line.left(colon).stripWhiteSpace();
            value = line.mid(colon + 1).stripWhiteSpace();
        }
        if (section == "System Locale") {
            int eq = value.find('=');
            if (eq > 0)
                m_cur[value.left(eq)] = value.mid(eq + 1);
        } else if (section == "VC Keymap") {
            m_cur["vc"] = value == "(unset)" || value == "n/a" ? TQString::null : value;
        } else if (section == "X11 Layout") {
            m_cur["layout"] = value == "(unset)" || value == "n/a" ? TQString::null : value;
        } else if (section == "X11 Model") {
            m_cur["model"] = value;
        } else if (section == "X11 Variant") {
            m_cur["variant"] = value;
        } else if (section == "X11 Options") {
            m_cur["options"] = value;
        }
    }

    for (TQMap<TQString, TQComboBox *>::Iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        setComboValue(*it, m_cur[it.key()]);
    setComboValue(m_vcKeymap, m_cur["vc"]);
    setComboValue(m_x11Layout, m_cur["layout"]);
    slotLayoutChanged();
    setComboValue(m_x11Model, m_cur["model"]);
    setComboValue(m_x11Variant, m_cur["variant"]);
    setComboValue(m_x11Options, m_cur["options"]);
    m_apply->setEnabled(false);
}

void LocalePage::slotLayoutChanged()
{
    TQString layout = m_x11Layout->currentText().stripWhiteSpace();
    TQString variant = m_x11Variant->currentText();
    m_x11Variant->blockSignals(true);
    m_x11Variant->clear();
    m_x11Variant->insertItem(TQString::null);
    if (!layout.isEmpty() && !layout.contains(',')) {
        m_x11Variant->insertStringList(SdProcess::tool("localectl", SystemScope,
            TQStringList("list-x11-keymap-variants") << "--no-pager" << layout).lines());
    }
    m_x11Variant->setEditText(variant);
    m_x11Variant->blockSignals(false);
    slotChanged();
}

void LocalePage::slotChanged()
{
    bool changed = false;
    for (TQMap<TQString, TQComboBox *>::Iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        changed |= comboValue(*it) != m_cur[it.key()];
    changed |= comboValue(m_vcKeymap) != m_cur["vc"];
    changed |= comboValue(m_x11Layout) != m_cur["layout"];
    changed |= comboValue(m_x11Model) != m_cur["model"];
    changed |= comboValue(m_x11Variant) != m_cur["variant"];
    changed |= comboValue(m_x11Options) != m_cur["options"];
    m_apply->setEnabled(changed);
}

void LocalePage::slotApply()
{
    bool localeChanged = false;
    TQStringList assignments;
    for (TQMap<TQString, TQComboBox *>::Iterator it = m_vars.begin(); it != m_vars.end(); ++it) {
        TQString v = comboValue(*it);
        if (v != m_cur[it.key()])
            localeChanged = true;
        if (!v.isEmpty())
            assignments << it.key() + "=" + v;
    }
    if (localeChanged) {
        if (assignments.isEmpty()) {
            KMessageBox::sorry(this, i18n("Please select at least a language (LANG)."));
            return;
        }
        SdResult r = SdProcess::privileged("locale", TQStringList("set-locale") += assignments);
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not change the system locale."), r);
    }

    TQString vc = comboValue(m_vcKeymap);
    if (vc != m_cur["vc"] && !vc.isEmpty()) {
        SdResult r = SdProcess::privileged("locale", TQStringList("set-keymap") << vc);
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not change the console keymap."), r);
    }

    TQString layout = comboValue(m_x11Layout);
    if (!layout.isEmpty() &&
        (layout != m_cur["layout"] || comboValue(m_x11Model) != m_cur["model"] ||
         comboValue(m_x11Variant) != m_cur["variant"] || comboValue(m_x11Options) != m_cur["options"])) {
        SdResult r = SdProcess::privileged("locale", TQStringList("set-x11-keymap") << layout << comboValue(m_x11Model)
                                                     << comboValue(m_x11Variant) << comboValue(m_x11Options));
        if (!r.ok())
            SdProcess::showError(this, i18n("Could not change the X11 keyboard layout."), r);
    }
    refresh();
}

#include "localepage.moc"
