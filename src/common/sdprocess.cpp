/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "sdprocess.h"

#include <config.h>

#include <tqapplication.h>
#include <tqeventloop.h>
#include <tqfile.h>
#include <tqfileinfo.h>

#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kprocess.h>

#include <unistd.h>

TQString SdResult::errorText() const
{
    TQString e = TQString::fromUtf8(err).stripWhiteSpace();
    if (!started)
        return i18n("The command could not be started.");
    if (exitCode == 126 || exitCode == 127) {
        if (e.isEmpty())
            return i18n("Authorization failed or was cancelled.");
    }
    if (e.isEmpty())
        e = TQString::fromUtf8(out).stripWhiteSpace();
    if (e.isEmpty())
        e = i18n("The command failed with exit code %1.").arg(exitCode);
    return e;
}

TQStringList SdResult::lines() const
{
    return TQStringList::split('\n', output());
}

// --------------------------------------------------------------------------


SdCollector::SdCollector(TDEProcess *p, SdResult *r)
    : TQObject(0, "SdCollector"), m_result(r), m_done(false)
{
    connect(p, TQ_SIGNAL(receivedStdout(TDEProcess *, char *, int)),
            this, TQ_SLOT(onOut(TDEProcess *, char *, int)));
    connect(p, TQ_SIGNAL(receivedStderr(TDEProcess *, char *, int)),
            this, TQ_SLOT(onErr(TDEProcess *, char *, int)));
    connect(p, TQ_SIGNAL(processExited(TDEProcess *)), this, TQ_SLOT(onExit(TDEProcess *)));
}

void SdCollector::onOut(TDEProcess *, char *buf, int len)
{
    m_result->out += TQCString(buf, len + 1);
}

void SdCollector::onErr(TDEProcess *, char *buf, int len)
{
    m_result->err += TQCString(buf, len + 1);
}

void SdCollector::onExit(TDEProcess *p)
{
    m_result->exitCode = p->normalExit() ? p->exitStatus() : -1;
    m_done = true;
}

// --------------------------------------------------------------------------

TQString SdProcess::findTool(const TQString &name)
{
    if (name.startsWith("/"))
        return TQFileInfo(name).isExecutable() ? name : TQString::null;
    static const char *const dirs[] = { "/usr/bin/", "/bin/", "/usr/sbin/", "/sbin/", 0 };
    for (int i = 0; dirs[i]; ++i) {
        TQString path = TQString(dirs[i]) + name;
        if (TQFileInfo(path).isExecutable())
            return path;
    }
    return TQString::null;
}

SdResult SdProcess::run(const TQStringList &argv)
{
    SdResult r;
    if (argv.isEmpty())
        return r;
    TQString exe = findTool(argv[0]);
    if (exe.isNull())
        return r;

    TDEProcess proc;
    proc.setEnvironment("LC_ALL", "C.UTF-8");
    proc.setEnvironment("SYSTEMD_PAGER", "");
    proc.setEnvironment("SYSTEMD_COLORS", "0");
    proc << exe;
    for (TQStringList::ConstIterator it = ++argv.begin(); it != argv.end(); ++it)
        proc << *it;

    SdCollector collector(&proc, &r);
    if (!proc.start(TDEProcess::NotifyOnExit, TDEProcess::AllOutput))
        return r;
    r.started = true;

    TQApplication::setOverrideCursor(TQt::waitCursor);
    while (!collector.done())
        tqApp->eventLoop()->processEvents(TQEventLoop::ExcludeUserInput | TQEventLoop::WaitForMore);
    TQApplication::restoreOverrideCursor();
    return r;
}

SdResult SdProcess::tool(const TQString &name, SdScope scope, const TQStringList &args)
{
    TQStringList argv;
    argv << name;
    if (scope == UserScope)
        argv << "--user";
    argv += args;
    return run(argv);
}

bool SdProcess::privilegedAvailable()
{
    return !findTool("pkexec").isNull() && TQFileInfo(TDESYSTEMD_HELPER).isExecutable();
}

SdResult SdProcess::privileged(const TQString &category, const TQStringList &args)
{
    TQStringList argv;
    if (getuid() == 0) {
        argv << TDESYSTEMD_HELPER;
    } else {
        argv << "pkexec" << "--disable-internal-agent" << TDESYSTEMD_HELPER;
    }
    argv << category;
    argv += args;
    return run(argv);
}

bool SdProcess::hasSystemdService(const TQString &service)
{
    static const char *const dirs[] = {
        "/usr/share/dbus-1/system-services/", "/usr/local/share/dbus-1/system-services/", 0
    };
    for (int i = 0; dirs[i]; ++i) {
        if (TQFile::exists(TQString(dirs[i]) + "org.freedesktop." + service + ".service"))
            return true;
    }
    return false;
}

void SdProcess::showError(TQWidget *parent, const TQString &what, const SdResult &r)
{
    KMessageBox::detailedError(parent, what, r.errorText());
}

// --------------------------------------------------------------------------

SdLineReader::SdLineReader(TQObject *parent, const char *name)
    : TQObject(parent, name), m_proc(0)
{
}

SdLineReader::~SdLineReader()
{
    stop();
}

bool SdLineReader::start(const TQStringList &argv)
{
    stop();
    TQString exe = SdProcess::findTool(argv[0]);
    if (exe.isNull())
        return false;

    m_buffer.resize(0);
    m_err.resize(0);
    m_proc = new TDEProcess(this);
    m_proc->setEnvironment("LC_ALL", "C.UTF-8");
    m_proc->setEnvironment("SYSTEMD_PAGER", "");
    *m_proc << exe;
    for (TQStringList::ConstIterator it = ++argv.begin(); it != argv.end(); ++it)
        *m_proc << *it;
    connect(m_proc, TQ_SIGNAL(receivedStdout(TDEProcess *, char *, int)),
            this, TQ_SLOT(slotStdout(TDEProcess *, char *, int)));
    connect(m_proc, TQ_SIGNAL(receivedStderr(TDEProcess *, char *, int)),
            this, TQ_SLOT(slotStderr(TDEProcess *, char *, int)));
    connect(m_proc, TQ_SIGNAL(processExited(TDEProcess *)), this, TQ_SLOT(slotExited(TDEProcess *)));
    if (!m_proc->start(TDEProcess::NotifyOnExit, TDEProcess::AllOutput)) {
        delete m_proc;
        m_proc = 0;
        return false;
    }
    return true;
}

void SdLineReader::stop()
{
    if (m_proc) {
        m_proc->disconnect(this);
        m_proc->kill();
        delete m_proc;
        m_proc = 0;
    }
}

bool SdLineReader::isRunning() const
{
    return m_proc && m_proc->isRunning();
}

void SdLineReader::slotStdout(TDEProcess *, char *buffer, int len)
{
    m_buffer += TQCString(buffer, len + 1);
    int pos;
    while ((pos = m_buffer.find('\n')) >= 0) {
        TQCString line = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);
        if (!line.isEmpty())
            emit lineReceived(line);
    }
}

void SdLineReader::slotStderr(TDEProcess *, char *buffer, int len)
{
    m_err += TQCString(buffer, len + 1);
}

void SdLineReader::slotExited(TDEProcess *p)
{
    if (!m_buffer.isEmpty()) {
        emit lineReceived(m_buffer);
        m_buffer.resize(0);
    }
    int code = p->normalExit() ? p->exitStatus() : -1;
    TQString err = TQString::fromUtf8(m_err).stripWhiteSpace();
    m_proc->deleteLater();
    m_proc = 0;
    emit finished(code, err);
}

#include "sdprocess.moc"
