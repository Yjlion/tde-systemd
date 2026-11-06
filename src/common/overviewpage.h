/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_OVERVIEWPAGE_H
#define TDESYSTEMD_OVERVIEWPAGE_H

#include "sdpage.h"

#include <tqmap.h>

class TQLabel;
class TQTextEdit;
class TDEListView;
class KPushButton;

/** Summary of the system: state, boot time, versions and failed units. */
class OverviewPage : public SdPage
{
    TQ_OBJECT

public:
    OverviewPage(TQWidget *parent = 0, const char *name = 0);
    TQString title() const;
    TQString iconName() const;

protected:
    void doRefresh();

private slots:
    void slotResetFailed();
    void slotDaemonReload();

private:
    TQLabel *addRow(TQWidget *parent, class TQGridLayout *grid, int row, const TQString &label);

    TQLabel *m_stateIcon;
    TQLabel *m_state;
    TQMap<TQString, TQLabel *> m_values;
    TDEListView *m_failed;
    TQTextEdit *m_blame;
};

#endif
