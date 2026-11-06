/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_SDUNITS_H
#define TDESYSTEMD_SDUNITS_H

#include "sdprocess.h"

#include <tqstring.h>
#include <tqvaluelist.h>

class TQWidget;

struct SdUnit
{
    TQString name;
    TQString load;
    TQString active;
    TQString sub;
    TQString description;
    TQString fileState;
    TQString preset;

    TQString type() const { return name.section('.', -1); }
};

typedef TQValueList<SdUnit> SdUnitList;

namespace SdUnits
{
    /** List loaded units and installed unit files. @p type may be empty for all types. */
    SdUnitList list(SdScope scope, const TQString &type, TQString *error = 0);

    /**
     * Run a unit operation (start, stop, restart, reload, enable, disable,
     * mask, unmask, reset-failed, ...). System scope operations go through
     * polkit; errors are shown to the user. Returns true on success.
     */
    bool action(TQWidget *parent, SdScope scope, const TQString &verb, const TQString &unit);

    /** Manager wide operations: daemon-reload, reset-failed. */
    bool managerAction(TQWidget *parent, SdScope scope, const TQString &verb);

    /** Translated, human readable name of a unit operation. */
    TQString verbText(const TQString &verb);

    /** Format a timestamp in microseconds since the epoch. */
    TQString formatUsec(unsigned long long usec);

    /** Format a duration in microseconds as "3h 5min". */
    TQString formatDuration(long long usec);
}

#endif
