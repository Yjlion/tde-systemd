/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_MAINTENANCEDIALOG_H
#define TDESYSTEMD_MAINTENANCEDIALOG_H

#include <kdialogbase.h>

class TQButtonGroup;
class TQComboBox;
class TQLabel;
class TQRadioButton;
class TQSpinBox;
class TQTextEdit;
struct SdResult;

/** Journal maintenance: disk usage, vacuum, rotate, flush, sync and verify. */
class MaintenanceDialog : public KDialogBase
{
    TQ_OBJECT

public:
    MaintenanceDialog(TQWidget *parent = 0, const char *name = 0);

    /** True if an operation changed the journal files. */
    bool changedJournal() const { return m_changed; }

private slots:
    void updateUsage();
    void slotVacuum();
    void slotRotate();
    void slotFlush();
    void slotSync();
    void slotVerify();
    void slotShowConfig();

private:
    void runOp(const TQString &title, const TQStringList &args, bool modifies);

    TQLabel *m_usage;
    TQButtonGroup *m_mode;
    TQSpinBox *m_size;
    TQSpinBox *m_age;
    TQComboBox *m_ageUnit;
    TQSpinBox *m_files;
    TQTextEdit *m_output;
    bool m_changed;
};

#endif
