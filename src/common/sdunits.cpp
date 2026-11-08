/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "sdunits.h"
#include "sdpage.h"

#include <tqdatetime.h>
#include <tqmap.h>
#include <tqregexp.h>
#include <tqstringlist.h>
#include <tqtextedit.h>

#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <tdelocale.h>

TQTextEdit *sdCreateTextView(TQWidget *parent)
{
    TQTextEdit *t = new TQTextEdit(parent);
    t->setReadOnly(true);
    t->setTextFormat(TQt::PlainText);
    t->setWordWrap(TQTextEdit::NoWrap);
    t->setFont(TDEGlobalSettings::fixedFont());
    return t;
}

TQString sdStripSymbols(const TQString &text)
{
    TQString out;
    for (uint i = 0; i < text.length(); ++i) {
        TQChar c = text[i];
        ushort u = c.unicode();
        if ((u >= 0xd800 && u <= 0xdfff) || c.category() == TQChar::Symbol_Other || u == 0xfe0f)
            continue;
        out += c;
    }
    return out.stripWhiteSpace();
}

static TQString stripBullet(TQString line)
{
    line = line.stripWhiteSpace();
    // systemctl prefixes failed / not-found units with a bullet even in --plain mode
    while (line.startsWith(TQString::fromUtf8("\xe2\x97\x8f")) || line.startsWith("*"))
        line = line.mid(1).stripWhiteSpace();
    return line;
}

SdUnitList SdUnits::list(SdScope scope, const TQString &type, TQString *error)
{
    TQStringList common;
    common << "--plain" << "--no-legend" << "--no-pager" << "--full";
    if (!type.isEmpty())
        common << ("--type=" + type);

    TQMap<TQString, SdUnit> units;
    TQStringList order;

    TQStringList args;
    args << "list-units" << "--all";
    args += common;
    SdResult r = SdProcess::tool("systemctl", scope, args);
    if (!r.ok()) {
        if (error)
            *error = r.errorText();
        return SdUnitList();
    }
    TQStringList lines = r.lines();
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        TQString line = stripBullet(*it);
        TQStringList f = TQStringList::split(' ', line.simplifyWhiteSpace());
        if (f.count() < 4)
            continue;
        SdUnit u;
        u.name = f[0];
        u.load = f[1];
        u.active = f[2];
        u.sub = f[3];
        u.description = line.section(TQRegExp("\\s+"), 4);
        units.insert(u.name, u);
        order << u.name;
    }

    args.clear();
    args << "list-unit-files";
    args += common;
    r = SdProcess::tool("systemctl", scope, args);
    lines = r.lines();
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        TQStringList f = TQStringList::split(' ', stripBullet(*it).simplifyWhiteSpace());
        if (f.count() < 2)
            continue;
        TQMap<TQString, SdUnit>::Iterator u = units.find(f[0]);
        if (u == units.end()) {
            // template units (foo@.service) cannot be loaded, keep them listed anyway
            SdUnit nu;
            nu.name = f[0];
            nu.load = "-";
            nu.active = "inactive";
            nu.sub = "dead";
            u = units.insert(nu.name, nu);
            order << nu.name;
        }
        (*u).fileState = f[1];
        if (f.count() > 2)
            (*u).preset = f[2];
    }

    order.sort();
    SdUnitList result;
    for (TQStringList::ConstIterator it = order.begin(); it != order.end(); ++it)
        result.append(units[*it]);
    return result;
}

TQString SdUnits::verbText(const TQString &verb)
{
    if (verb == "start") return i18n("start");
    if (verb == "stop") return i18n("stop");
    if (verb == "restart") return i18n("restart");
    if (verb == "reload") return i18n("reload");
    if (verb == "enable") return i18n("enable");
    if (verb == "disable") return i18n("disable");
    if (verb == "mask") return i18n("mask");
    if (verb == "unmask") return i18n("unmask");
    if (verb == "reset-failed") return i18n("reset the failed state of");
    if (verb == "kill") return i18n("kill");
    return verb;
}

bool SdUnits::action(TQWidget *parent, SdScope scope, const TQString &verb, const TQString &unit)
{
    SdResult r;
    if (scope == SystemScope)
        r = SdProcess::privileged("units", TQStringList(verb) << unit);
    else
        r = SdProcess::tool("systemctl", UserScope, TQStringList(verb) << "--" << unit);
    if (!r.ok()) {
        SdProcess::showError(parent, i18n("Could not %1 the unit <b>%2</b>.").arg(verbText(verb)).arg(unit), r);
        return false;
    }
    return true;
}

bool SdUnits::managerAction(TQWidget *parent, SdScope scope, const TQString &verb)
{
    SdResult r;
    if (scope == SystemScope)
        r = SdProcess::privileged("units", TQStringList(verb));
    else
        r = SdProcess::tool("systemctl", UserScope, TQStringList(verb));
    if (!r.ok()) {
        SdProcess::showError(parent, i18n("The operation \"%1\" failed.").arg(verb), r);
        return false;
    }
    return true;
}

TQString SdUnits::formatUsec(unsigned long long usec)
{
    if (usec == 0)
        return TQString::null;
    TQDateTime dt;
    dt.setTime_t((uint)(usec / 1000000ULL));
    return TDEGlobal::locale()->formatDateTime(dt, true, true);
}

TQString SdUnits::formatDuration(long long usec)
{
    bool neg = usec < 0;
    unsigned long long s = (neg ? -usec : usec) / 1000000ULL;
    TQString out;
    if (s >= 86400) {
        out = i18n("%1d %2h").arg(s / 86400).arg((s % 86400) / 3600);
    } else if (s >= 3600) {
        out = i18n("%1h %2min").arg(s / 3600).arg((s % 3600) / 60);
    } else if (s >= 60) {
        out = i18n("%1min %2s").arg(s / 60).arg(s % 60);
    } else {
        out = i18n("%1s").arg(s);
    }
    return neg ? i18n("%1 ago").arg(out) : out;
}
