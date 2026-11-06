/*
 * tde-systemd-helper - privileged helper for tde-systemd
 *
 * Copyright (C) 2026 tde-systemd developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is started through pkexec(1). It accepts a small, fixed set
 * of operations, validates every argument and then executes the matching
 * systemd tool directly (never through a shell). The polkit action that
 * authorises a call is chosen from the first argument (the category), see
 * org.trinitydesktop.tde-systemd.policy.
 *
 * Usage:
 *   tde-systemd-helper units    <verb> [unit]
 *   tde-systemd-helper journal  <verb> [value]
 *   tde-systemd-helper journal-read [filter options]
 *   tde-systemd-helper hostname <verb> <value>
 *   tde-systemd-helper timedate <verb> <value>
 *   tde-systemd-helper locale   <verb> <values...>
 */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace {

typedef std::vector<std::string> Args;

const int EXIT_USAGE = 64;   // EX_USAGE
const int EXIT_DENIED = 77;  // EX_NOPERM

int fail(int code, const std::string &msg)
{
    fprintf(stderr, "tde-systemd-helper: %s\n", msg.c_str());
    return code;
}

bool matches(const std::string &s, const char *re)
{
    return std::regex_match(s, std::regex(re));
}

// Printable text without control characters, not starting with '-'.
bool isSafeText(const std::string &s, size_t maxLen)
{
    if (s.empty() || s.size() > maxLen || s[0] == '-')
        return false;
    for (unsigned char c : s) {
        if (c < 0x20 || c == 0x7f)
            return false;
    }
    return true;
}

bool isUnitName(const std::string &s)
{
    if (s.size() > 256 || s.find("..") != std::string::npos)
        return false;
    return matches(s, "[A-Za-z0-9_:\\\\@][A-Za-z0-9_:.\\\\@-]*"
                      "\\.(service|socket|target|timer|mount|automount|path|swap|slice|scope|device)");
}

bool isBool(const std::string &s)
{
    return s == "true" || s == "false";
}

bool isWord(const std::string &s, size_t maxLen = 128)
{
    return !s.empty() && s.size() <= maxLen && matches(s, "[A-Za-z0-9_][A-Za-z0-9_.,+@-]*");
}

bool isTimezone(const std::string &s)
{
    if (s.empty() || s.size() > 128 || s.find("..") != std::string::npos)
        return false;
    if (!matches(s, "[A-Za-z0-9_][A-Za-z0-9_+/-]*"))
        return false;
    struct stat st;
    std::string path = "/usr/share/zoneinfo/" + s;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

const char *findTool(const char *name)
{
    static const char *dirs[] = { "/usr/bin/", "/bin/", "/usr/sbin/", "/sbin/", nullptr };
    static std::string found;
    for (int i = 0; dirs[i]; ++i) {
        found = std::string(dirs[i]) + name;
        if (access(found.c_str(), X_OK) == 0)
            return found.c_str();
    }
    return nullptr;
}

int run(const char *tool, const Args &args)
{
    const char *path = findTool(tool);
    if (!path)
        return fail(127, std::string("cannot find ") + tool);

    std::string toolPath(path);
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>(toolPath.c_str()));
    for (const std::string &a : args)
        argv.push_back(const_cast<char *>(a.c_str()));
    argv.push_back(nullptr);

    static char envPath[] = "PATH=/usr/sbin:/usr/bin:/sbin:/bin";
    static char envLang[] = "LC_ALL=C.UTF-8";
    static char envPager[] = "SYSTEMD_PAGER=";
    static char envColors[] = "SYSTEMD_COLORS=0";
    char *envp[] = { envPath, envLang, envPager, envColors, nullptr };

    execve(toolPath.c_str(), argv.data(), envp);
    return fail(126, std::string("exec failed: ") + strerror(errno));
}

// ---------------------------------------------------------------- units

int doUnits(const Args &a)
{
    if (a.size() == 1 && (a[0] == "daemon-reload" || a[0] == "reset-failed"))
        return run("systemctl", { "--no-ask-password", a[0] });

    static const std::set<std::string> verbs = {
        "start", "stop", "restart", "reload", "try-restart", "reload-or-restart",
        "enable", "disable", "reenable", "mask", "unmask", "reset-failed", "kill"
    };
    if (a.size() != 2 || !verbs.count(a[0]))
        return fail(EXIT_USAGE, "units: invalid operation");
    if (!isUnitName(a[1]))
        return fail(EXIT_USAGE, "units: invalid unit name");
    return run("systemctl", { "--no-ask-password", a[0], "--", a[1] });
}

// ---------------------------------------------------------------- journal

int doJournal(const Args &a)
{
    if (a.size() == 1) {
        if (a[0] == "rotate" || a[0] == "flush" || a[0] == "sync")
            return run("journalctl", { "--" + a[0] });
        if (a[0] == "verify")
            return run("journalctl", { "--verify", "--no-pager" });
        if (a[0] == "disk-usage")
            return run("journalctl", { "--disk-usage" });
    }
    if (a.size() == 2) {
        if (a[0] == "vacuum-size" && matches(a[1], "[0-9]{1,9}[KMGT]?"))
            return run("journalctl", { "--vacuum-size=" + a[1] });
        if (a[0] == "vacuum-time" && matches(a[1], "[0-9]{1,9}(s|min|h|d|weeks|months|years)"))
            return run("journalctl", { "--vacuum-time=" + a[1] });
        if (a[0] == "vacuum-files" && matches(a[1], "[0-9]{1,6}"))
            return run("journalctl", { "--vacuum-files=" + a[1] });
    }
    return fail(EXIT_USAGE, "journal: invalid operation");
}

// Read the full system journal as root. Only filter options are accepted;
// the output format is fixed to JSON and following (-f) is not allowed.
int doJournalRead(const Args &a)
{
    Args out = { "--no-pager", "-o", "json" };
    for (size_t i = 0; i < a.size(); ++i) {
        const std::string &opt = a[i];
        if (opt == "-k" || opt == "--dmesg") {
            out.push_back("-k");
            continue;
        }
        if (opt == "-r") {
            out.push_back("-r");
            continue;
        }
        static const std::set<std::string> valueOptions = { "-b", "-p", "-u", "-t", "-S", "-U", "-g", "-n" };
        if (!valueOptions.count(opt))
            return fail(EXIT_USAGE, "journal-read: option not allowed: " + opt);
        if (i + 1 >= a.size())
            return fail(EXIT_USAGE, "journal-read: missing value for " + opt);
        const std::string &v = a[++i];
        bool ok = false;
        if (opt == "-b")
            ok = matches(v, "-?[0-9]{1,5}|[0-9a-f]{32}");
        else if (opt == "-p")
            ok = matches(v, "[0-7](\\.\\.[0-7])?|emerg|alert|crit|err|warning|notice|info|debug");
        else if (opt == "-u")
            ok = isSafeText(v, 256) && matches(v, "[A-Za-z0-9_:.\\\\@*-]+");
        else if (opt == "-t")
            ok = isSafeText(v, 256);
        else if (opt == "-S" || opt == "-U")
            ok = isSafeText(v, 64) && matches(v, "[A-Za-z0-9 :.+-]+");
        else if (opt == "-g")
            ok = !v.empty() && v.size() <= 512 && v.find('\n') == std::string::npos;
        else if (opt == "-n")
            ok = matches(v, "[0-9]{1,7}");
        if (!ok)
            return fail(EXIT_USAGE, "journal-read: invalid option " + opt);
        // always use the attached long form, values may start with '-'
        static const char *const longNames[][2] = {
            { "-b", "--boot=" }, { "-p", "--priority=" }, { "-u", "--unit=" },
            { "-t", "--identifier=" }, { "-S", "--since=" }, { "-U", "--until=" },
            { "-g", "--grep=" }, { "-n", "--lines=" }
        };
        for (const auto &ln : longNames) {
            if (opt == ln[0])
                out.push_back(std::string(ln[1]) + v);
        }
    }
    return run("journalctl", out);
}

// ---------------------------------------------------------------- hostname

int doHostname(const Args &a)
{
    if (a.size() != 2)
        return fail(EXIT_USAGE, "hostname: invalid operation");
    const std::string &verb = a[0], &v = a[1];

    if (verb == "set-hostname") {
        if (v.size() <= 64 && matches(v, "[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?(\\.[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?)*"))
            return run("hostnamectl", { "--no-ask-password", "--static", "hostname", v });
    } else if (verb == "set-pretty") {
        if (v.empty() || isSafeText(v, 255))
            return run("hostnamectl", { "--no-ask-password", "--pretty", "hostname", "--", v });
    } else if (verb == "set-icon") {
        if (v.empty() || isWord(v, 64))
            return run("hostnamectl", { "--no-ask-password", "icon-name", "--", v });
    } else if (verb == "set-chassis") {
        static const std::set<std::string> chassis = {
            "", "desktop", "laptop", "convertible", "server", "tablet",
            "handset", "watch", "embedded", "vm", "container"
        };
        if (chassis.count(v))
            return run("hostnamectl", { "--no-ask-password", "chassis", "--", v });
    } else if (verb == "set-deployment") {
        if (v.empty() || isWord(v, 64))
            return run("hostnamectl", { "--no-ask-password", "deployment", "--", v });
    } else if (verb == "set-location") {
        if (v.empty() || isSafeText(v, 255))
            return run("hostnamectl", { "--no-ask-password", "location", "--", v });
    }
    return fail(EXIT_USAGE, "hostname: invalid operation or value");
}

// ---------------------------------------------------------------- timedate

int doTimedate(const Args &a)
{
    if (a.size() != 2)
        return fail(EXIT_USAGE, "timedate: invalid operation");
    const std::string &verb = a[0], &v = a[1];

    if (verb == "set-time" && matches(v, "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}(:[0-9]{2})?"))
        return run("timedatectl", { "--no-ask-password", "set-time", v });
    if (verb == "set-timezone" && isTimezone(v))
        return run("timedatectl", { "--no-ask-password", "set-timezone", v });
    if (verb == "set-ntp" && isBool(v))
        return run("timedatectl", { "--no-ask-password", "set-ntp", v });
    if (verb == "set-local-rtc" && isBool(v))
        return run("timedatectl", { "--no-ask-password", "set-local-rtc", v });
    return fail(EXIT_USAGE, "timedate: invalid operation or value");
}

// ---------------------------------------------------------------- locale

int doLocale(const Args &a)
{
    if (a.empty())
        return fail(EXIT_USAGE, "locale: missing operation");
    const std::string &verb = a[0];

    if (verb == "set-locale") {
        static const std::set<std::string> vars = {
            "LANG", "LANGUAGE", "LC_CTYPE", "LC_NUMERIC", "LC_TIME", "LC_COLLATE",
            "LC_MONETARY", "LC_MESSAGES", "LC_PAPER", "LC_NAME", "LC_ADDRESS",
            "LC_TELEPHONE", "LC_MEASUREMENT", "LC_IDENTIFICATION"
        };
        if (a.size() < 2 || a.size() > vars.size() + 1)
            return fail(EXIT_USAGE, "locale: invalid number of assignments");
        Args out = { "--no-ask-password", "set-locale" };
        for (size_t i = 1; i < a.size(); ++i) {
            size_t eq = a[i].find('=');
            if (eq == std::string::npos || !vars.count(a[i].substr(0, eq)))
                return fail(EXIT_USAGE, "locale: invalid variable");
            std::string value = a[i].substr(eq + 1);
            if (!matches(value, "[A-Za-z0-9_.@:-]{1,64}"))
                return fail(EXIT_USAGE, "locale: invalid value");
            out.push_back(a[i]);
        }
        return run("localectl", out);
    }
    if (verb == "set-keymap" && a.size() == 2 && isWord(a[1], 64))
        return run("localectl", { "--no-ask-password", "set-keymap", a[1] });
    if (verb == "set-x11-keymap" && a.size() >= 2 && a.size() <= 5) {
        Args out = { "--no-ask-password", "set-x11-keymap" };
        for (size_t i = 1; i < a.size(); ++i) {
            // empty values are allowed for model / variant / options
            if (!a[i].empty() && !matches(a[i], "[A-Za-z0-9_][A-Za-z0-9_.,:+()-]{0,255}"))
                return fail(EXIT_USAGE, "locale: invalid keymap value");
            out.push_back(a[i]);
        }
        if (a[1].empty())
            return fail(EXIT_USAGE, "locale: layout must not be empty");
        return run("localectl", out);
    }
    return fail(EXIT_USAGE, "locale: invalid operation");
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2)
        return fail(EXIT_USAGE, "usage: tde-systemd-helper <category> <operation> [arguments]");

    if (geteuid() != 0)
        return fail(EXIT_DENIED, "must be started as root through pkexec");

    std::string category = argv[1];
    Args args(argv + 2, argv + argc);

    if (category == "units")
        return doUnits(args);
    if (category == "journal")
        return doJournal(args);
    if (category == "journal-read")
        return doJournalRead(args);
    if (category == "hostname")
        return doHostname(args);
    if (category == "timedate")
        return doTimedate(args);
    if (category == "locale")
        return doLocale(args);
    return fail(EXIT_USAGE, "unknown category " + category);
}
