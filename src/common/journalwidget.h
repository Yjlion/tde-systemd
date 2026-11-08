/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_JOURNALWIDGET_H
#define TDESYSTEMD_JOURNALWIDGET_H

#include "sdprocess.h"

#include <tqwidget.h>

class TQCheckBox;
class TQComboBox;
class TQLabel;
class TQSpinBox;
class TQTextEdit;
class TQTimer;
class KLineEdit;
class KPushButton;
class TDEListView;
class TDEListViewSearchLine;

/**
 * Journal viewer: filters, the entry list and a detail view of the selected
 * entry. Used by the stand-alone viewer and by the Control Center module.
 */
class JournalWidget : public TQWidget
{
    TQ_OBJECT

public:
    JournalWidget(TQWidget *parent = 0, const char *name = 0);
    ~JournalWidget();

    SdScope scope() const;
    bool isFollowing() const;

    /** Preset the unit filter, e.g. from the command line. */
    void setUnitFilter(const TQString &unit);
    void setScope(SdScope scope);
    void setPriorityFilter(int priority);
    void setBootFilter(const TQString &offset);

signals:
    void followChanged(bool on);

public slots:
    void reload();
    void setFollow(bool on);
    void toggleFollow();
    void exportLog();
    void openMaintenance();
    void clearFilters();

private slots:
    void slotScopeChanged();
    void slotLine(const TQCString &line);
    void slotFinished(int exitCode, const TQString &error);
    void slotSelectionChanged();
    void slotFlush();

private:
    TQStringList filterArgs(bool forHelper) const;
    void loadBoots();
    void loadUnits();
    void startReader(bool follow);

    TQComboBox *m_scope;
    TQComboBox *m_boot;
    TQComboBox *m_priority;
    TQComboBox *m_unit;
    KLineEdit *m_ident;
    KLineEdit *m_since;
    KLineEdit *m_until;
    KLineEdit *m_grep;
    TQSpinBox *m_lines;
    TQCheckBox *m_kernel;
    TQCheckBox *m_admin;
    KPushButton *m_follow;
    TDEListViewSearchLine *m_search;
    TDEListView *m_list;
    TQTextEdit *m_detail;
    TQLabel *m_status;

    SdLineReader *m_reader;
    TQTimer *m_flushTimer;
    TQValueList<TQCString> m_pending;
    TQString m_lastCursor;
    bool m_following;
    bool m_loadingThenFollow;
    int m_count;
};

#endif
