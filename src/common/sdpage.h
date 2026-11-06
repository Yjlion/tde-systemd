/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_SDPAGE_H
#define TDESYSTEMD_SDPAGE_H

#include <tqwidget.h>

/** Base class of the pages shown in the systemd manager. Pages load lazily. */
class SdPage : public TQWidget
{
    TQ_OBJECT

public:
    SdPage(TQWidget *parent = 0, const char *name = 0)
        : TQWidget(parent, name), m_loaded(false), m_refreshing(false) {}

    /** Load the data if this has not happened yet. */
    void ensureLoaded()
    {
        if (!m_loaded) {
            m_loaded = true;
            refresh();
        }
    }

    bool isLoaded() const { return m_loaded; }

    virtual TQString title() const = 0;
    virtual TQString iconName() const = 0;

public slots:
    /**
     * Reload the data of the page. Commands keep the event loop running, so
     * a second request while one is in progress is ignored.
     */
    void refresh()
    {
        if (m_refreshing)
            return;
        m_refreshing = true;
        doRefresh();
        m_refreshing = false;
    }

protected:
    virtual void doRefresh() = 0;
    void markLoaded() { m_loaded = true; }

private:
    bool m_loaded;
    bool m_refreshing;
};

/** A read-only text view using the fixed width font. */
class TQTextEdit;
TQTextEdit *sdCreateTextView(TQWidget *parent);

/** Remove emoji and other pictographs that tools like hostnamectl print. */
TQString sdStripSymbols(const TQString &text);

#endif
