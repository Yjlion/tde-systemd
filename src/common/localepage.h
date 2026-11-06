/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_LOCALEPAGE_H
#define TDESYSTEMD_LOCALEPAGE_H

#include "sdpage.h"

#include <tqmap.h>
#include <tqstringlist.h>

class TQComboBox;
class KPushButton;

/** System locale and keyboard settings through systemd-localed. */
class LocalePage : public SdPage
{
    TQ_OBJECT

public:
    LocalePage(TQWidget *parent = 0, const char *name = 0);
    TQString title() const;
    TQString iconName() const;

    static bool isAvailable();

protected:
    void doRefresh();

private slots:
    void slotChanged();
    void slotLayoutChanged();
    void slotApply();

private:
    TQComboBox *makeCombo(TQWidget *parent, bool editable);
    void setComboValue(TQComboBox *c, const TQString &value);
    TQString comboValue(TQComboBox *c) const;

    TQMap<TQString, TQComboBox *> m_vars;   // LANG, LC_*
    TQComboBox *m_vcKeymap;
    TQComboBox *m_x11Layout;
    TQComboBox *m_x11Model;
    TQComboBox *m_x11Variant;
    TQComboBox *m_x11Options;
    KPushButton *m_apply;

    TQMap<TQString, TQString> m_cur;
    bool m_listsLoaded;
};

#endif
