/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_SERVICESPAGE_H
#define TDESYSTEMD_SERVICESPAGE_H

#include "sdpage.h"
#include "sdunits.h"

class TQComboBox;
class TQLabel;
class TQListViewItem;
class TQPoint;
class TQTabWidget;
class TQTextEdit;
class KPushButton;
class TDEListView;
class TDEListViewSearchLine;

/** List of system or user units with start/stop/enable/... operations. */
class ServicesPage : public SdPage
{
    TQ_OBJECT

public:
    ServicesPage(TQWidget *parent = 0, const char *name = 0);

    TQString title() const;
    TQString iconName() const;

    SdScope scope() const;

    /** Show and select a unit. */
    void selectUnit(const TQString &unit, SdScope scope);

protected:
    void doRefresh();

private slots:
    void slotSelectionChanged();
    void slotDetailTabChanged();
    void slotContextMenu(TDEListView *, TQListViewItem *, const TQPoint &);
    void slotFilterChanged();
    void slotAction(int id);
    void slotStart();
    void slotStop();
    void slotRestart();
    void slotReload();
    void slotEnable();
    void slotDisable();
    void slotMask();
    void slotUnmask();
    void slotDaemonReload();

private:
    TQString currentUnit() const;
    void runAction(const TQString &verb);
    void updateButtons();
    void loadDetails();
    void fillList();

    TQComboBox *m_scope;
    TQComboBox *m_type;
    TQComboBox *m_state;
    TDEListViewSearchLine *m_search;
    TDEListView *m_list;
    TQLabel *m_summary;
    TQTabWidget *m_details;
    TQTextEdit *m_status;
    TQTextEdit *m_file;
    TQTextEdit *m_props;
    TQTextEdit *m_deps;
    TQTextEdit *m_log;
    KPushButton *m_start, *m_stop, *m_restart, *m_reload;
    KPushButton *m_enable, *m_disable, *m_mask, *m_unmask, *m_daemonReload;
    SdUnitList m_units;
    TQString m_detailsFor;
    TQString m_pendingSelect;
};

#endif
