/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_SDPROCESS_H
#define TDESYSTEMD_SDPROCESS_H

#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqcstring.h>

class TDEProcess;

/** Which systemd instance a request is addressed to. */
enum SdScope { SystemScope = 0, UserScope = 1 };

/** Result of a finished command. */
struct SdResult
{
    SdResult() : exitCode(-1), started(false) {}
    int exitCode;
    bool started;
    TQCString out;
    TQCString err;

    bool ok() const { return started && exitCode == 0; }
    TQString output() const { return TQString::fromUtf8(out); }
    TQString errorText() const;
    TQStringList lines() const;
};

/**
 * Runs systemd command line tools.
 *
 * Commands run without blocking the event loop: while a command runs, paint
 * and timer events are still processed but user input is held back, so the
 * GUI stays responsive (e.g. while the polkit agent asks for a password).
 */
class SdProcess
{
public:
    /** Run @p argv (argv[0] is looked up in the usual system paths). */
    static SdResult run(const TQStringList &argv);

    /** Run a systemd tool for the given scope ("--user" is added for UserScope). */
    static SdResult tool(const TQString &name, SdScope scope, const TQStringList &args);

    /**
     * Run a privileged operation through pkexec and tde-systemd-helper.
     * @p category selects the polkit action (units, journal, journal-read,
     * hostname, timedate, locale).
     */
    static SdResult privileged(const TQString &category, const TQStringList &args);

    /** Full path of a system tool or TQString::null if it is missing. */
    static TQString findTool(const TQString &name);

    /** True if pkexec and the helper are installed. */
    static bool privilegedAvailable();

    /** True if a D-Bus service of systemd (e.g. "timedate1") can be activated. */
    static bool hasSystemdService(const TQString &service);

    /** Show an error dialog for a failed result. */
    static void showError(TQWidget *parent, const TQString &what, const SdResult &r);
};

/**
 * Line oriented asynchronous reader, used for streaming output such as
 * "journalctl -f".
 */
class SdLineReader : public TQObject
{
    TQ_OBJECT

public:
    SdLineReader(TQObject *parent = 0, const char *name = 0);
    ~SdLineReader();

    bool start(const TQStringList &argv);
    void stop();
    bool isRunning() const;

signals:
    void lineReceived(const TQCString &line);
    void finished(int exitCode, const TQString &errorText);

private slots:
    void slotStdout(TDEProcess *, char *buffer, int len);
    void slotStderr(TDEProcess *, char *buffer, int len);
    void slotExited(TDEProcess *);

private:
    TDEProcess *m_proc;
    TQCString m_buffer;
    TQCString m_err;
};

/** Internal: collects the output of a TDEProcess into an SdResult. */
class SdCollector : public TQObject
{
    TQ_OBJECT

public:
    SdCollector(TDEProcess *p, SdResult *r);
    bool done() const { return m_done; }

private slots:
    void onOut(TDEProcess *, char *buf, int len);
    void onErr(TDEProcess *, char *buf, int len);
    void onExit(TDEProcess *p);

private:
    SdResult *m_result;
    bool m_done;
};

#endif
