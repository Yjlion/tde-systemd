/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "maintenancedialog.h"
#include "sdprocess.h"
#include "sdpage.h"

#include <tqbuttongroup.h>
#include <tqcombobox.h>
#include <tqdatetime.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqspinbox.h>
#include <tqtextedit.h>

#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kpushbutton.h>

MaintenanceDialog::MaintenanceDialog(TQWidget *parent, const char *name)
    : KDialogBase(parent, name, true, i18n("Journal Maintenance"), Close | Help, Close, true),
      m_changed(false)
{
    setHelp("journal-maintenance", "tde-systemd");
    TQWidget *page = new TQWidget(this);
    setMainWidget(page);
    TQVBoxLayout *top = new TQVBoxLayout(page, 0, 8);

    // ---- disk usage
    TQHBoxLayout *usageRow = new TQHBoxLayout(top, 8);
    TQLabel *icon = new TQLabel(page);
    icon->setPixmap(DesktopIcon("drive-harddisk-mounted", 32));
    usageRow->addWidget(icon);
    m_usage = new TQLabel(page);
    usageRow->addWidget(m_usage, 1);
    KPushButton *usageBtn = new KPushButton(SmallIconSet("reload"), i18n("Refresh"), page);
    usageRow->addWidget(usageBtn);

    // ---- vacuum
    TQGroupBox *vac = new TQGroupBox(i18n("Remove Old Entries (Vacuum)"), page);
    vac->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *vg = new TQGridLayout(vac->layout(), 4, 3, 6);
    m_mode = new TQButtonGroup(vac);
    m_mode->hide();
    TQRadioButton *bySize = new TQRadioButton(i18n("Reduce archived journal files to at most:"), vac);
    TQRadioButton *byAge = new TQRadioButton(i18n("Remove archived entries older than:"), vac);
    TQRadioButton *byFiles = new TQRadioButton(i18n("Keep only this number of archived files:"), vac);
    m_mode->insert(bySize, 0);
    m_mode->insert(byAge, 1);
    m_mode->insert(byFiles, 2);
    bySize->setChecked(true);
    vg->addWidget(bySize, 0, 0);
    m_size = new TQSpinBox(1, 1000000, 50, vac);
    m_size->setSuffix(i18n(" MB"));
    m_size->setValue(500);
    vg->addWidget(m_size, 0, 1);
    vg->addWidget(byAge, 1, 0);
    TQHBoxLayout *ageRow = new TQHBoxLayout(6);
    m_age = new TQSpinBox(1, 100000, 1, vac);
    m_age->setValue(4);
    m_ageUnit = new TQComboBox(vac);
    m_ageUnit->insertItem(i18n("days"));
    m_ageUnit->insertItem(i18n("weeks"));
    m_ageUnit->insertItem(i18n("months"));
    m_ageUnit->insertItem(i18n("years"));
    m_ageUnit->setCurrentItem(1);
    ageRow->addWidget(m_age);
    ageRow->addWidget(m_ageUnit);
    vg->addLayout(ageRow, 1, 1);
    vg->addWidget(byFiles, 2, 0);
    m_files = new TQSpinBox(1, 100000, 1, vac);
    m_files->setValue(10);
    vg->addWidget(m_files, 2, 1);
    KPushButton *vacBtn = new KPushButton(SmallIconSet("edittrash"), i18n("&Vacuum Now"), vac);
    vg->addWidget(vacBtn, 3, 1);
    TQLabel *vacNote = new TQLabel(i18n("Only archived journal files are removed; the active files are "
                                       "kept. Rotate the journal first to include recent entries."), vac);
    vacNote->setAlignment(TQt::WordBreak);
    vg->addMultiCellWidget(vacNote, 4, 4, 0, 2);
    vg->setColStretch(2, 1);
    top->addWidget(vac);

    // ---- other operations
    TQGroupBox *ops = new TQGroupBox(i18n("Operations"), page);
    ops->setColumnLayout(0, TQt::Vertical);
    TQGridLayout *og = new TQGridLayout(ops->layout(), 5, 2, 6);
    struct Op { const char *icon; TQString text; TQString help; const char *slot; };
    Op opsList[] = {
        { "reload", i18n("&Rotate"), i18n("Archive the active journal files and start new ones."), TQ_SLOT(slotRotate()) },
        { "document-save", i18n("F&lush"), i18n("Move the volatile journal from /run to persistent storage in /var."), TQ_SLOT(slotFlush()) },
        { "save_all", i18n("&Sync"), i18n("Write all pending entries to disk."), TQ_SLOT(slotSync()) },
        { "viewmag", i18n("V&erify"), i18n("Check the journal files for consistency."), TQ_SLOT(slotVerify()) },
        { "text-x-generic", i18n("Show &Configuration"), i18n("Show the active journald.conf including drop-in files."), TQ_SLOT(slotShowConfig()) },
    };
    for (int i = 0; i < 5; ++i) {
        KPushButton *b = new KPushButton(SmallIconSet(opsList[i].icon), opsList[i].text, ops);
        og->addWidget(b, i, 0);
        TQLabel *help = new TQLabel(opsList[i].help, ops);
        help->setAlignment(TQt::AlignVCenter | TQt::WordBreak);
        og->addWidget(help, i, 1);
        connect(b, TQ_SIGNAL(clicked()), this, opsList[i].slot);
    }
    og->setColStretch(1, 1);
    top->addWidget(ops);

    m_output = sdCreateTextView(page);
    m_output->setMinimumHeight(140);
    top->addWidget(m_output, 1);

    connect(usageBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(updateUsage()));
    connect(vacBtn, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotVacuum()));

    resize(760, 640);
    updateUsage();
}

void MaintenanceDialog::updateUsage()
{
    SdResult r = SdProcess::tool("journalctl", SystemScope, TQStringList("--disk-usage"));
    TQString text = r.output().stripWhiteSpace();
    if (!r.ok() || text.isEmpty())
        text = i18n("The disk usage could not be determined: %1").arg(r.errorText());
    m_usage->setText(text);
}

void MaintenanceDialog::runOp(const TQString &title, const TQStringList &args, bool modifies)
{
    m_output->append(TQString("[%1] %2").arg(TQTime::currentTime().toString()).arg(title));
    SdResult r = SdProcess::privileged("journal", args);
    TQString out = (r.output() + "\n" + TQString::fromUtf8(r.err)).stripWhiteSpace();
    if (!out.isEmpty())
        m_output->append(out);
    if (r.ok()) {
        m_output->append(i18n("Done."));
        if (modifies)
            m_changed = true;
    } else {
        m_output->append(i18n("Failed: %1").arg(r.errorText()));
    }
    m_output->append(TQString::null);
    m_output->scrollToBottom();
    updateUsage();
}

void MaintenanceDialog::slotVacuum()
{
    TQStringList args;
    TQString what;
    switch (m_mode->selectedId()) {
    case 0:
        args << "vacuum-size" << TQString("%1M").arg(m_size->value());
        what = i18n("Reduce the journal to %1 MB").arg(m_size->value());
        break;
    case 1: {
        static const char *const units[] = { "d", "weeks", "months", "years" };
        args << "vacuum-time" << TQString("%1%2").arg(m_age->value()).arg(units[m_ageUnit->currentItem()]);
        what = i18n("Remove entries older than %1 %2").arg(m_age->value()).arg(m_ageUnit->currentText());
        break;
    }
    default:
        args << "vacuum-files" << TQString::number(m_files->value());
        what = i18n("Keep %1 archived files").arg(m_files->value());
        break;
    }
    if (KMessageBox::warningContinueCancel(this, i18n("%1?\n\nRemoved journal entries cannot be restored.").arg(what),
                                           i18n("Vacuum Journal"), KGuiItem(i18n("&Vacuum"), "edittrash"))
        != KMessageBox::Continue)
        return;
    runOp(what, args, true);
}

void MaintenanceDialog::slotRotate() { runOp(i18n("Rotate journal files"), TQStringList("rotate"), true); }
void MaintenanceDialog::slotFlush() { runOp(i18n("Flush journal to persistent storage"), TQStringList("flush"), true); }
void MaintenanceDialog::slotSync() { runOp(i18n("Sync journal"), TQStringList("sync"), false); }
void MaintenanceDialog::slotVerify() { runOp(i18n("Verify journal files"), TQStringList("verify"), false); }

void MaintenanceDialog::slotShowConfig()
{
    SdResult r = SdProcess::run(TQStringList("systemd-analyze") << "cat-config" << "--no-pager" << "systemd/journald.conf");
    m_output->append(i18n("Active journald configuration:"));
    m_output->append(r.ok() ? r.output() : r.errorText());
    m_output->scrollToBottom();
}

#include "maintenancedialog.moc"
