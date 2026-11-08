/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TDESYSTEMD_SDJSON_H
#define TDESYSTEMD_SDJSON_H

#include <tqcstring.h>
#include <tqstring.h>
#include <tqvaluelist.h>

#include <map>
#include <vector>

/**
 * Minimal JSON value, sufficient for the output of systemd tools
 * ("journalctl -o json", "systemctl ... -o json").
 *
 * Children are kept in std containers: TQt3 containers allocate a value in
 * their header node on construction, which recurses for a recursive type.
 */
class SdJson
{
public:
    enum Type { Null, Bool, Number, String, Array, Object };

    SdJson() : m_type(Null), m_bool(false) {}

    static SdJson parse(const TQCString &text, bool *ok = 0);

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Null; }
    bool isArray() const { return m_type == Array; }
    bool isObject() const { return m_type == Object; }

    /** String form: strings as-is, numbers as written, byte arrays decoded as UTF-8. */
    TQString toString() const;
    bool toBool() const { return m_type == Bool ? m_bool : toString() == "true"; }
    unsigned long long toULongLong() const { return toString().toULongLong(); }

    typedef std::vector<SdJson> List;
    const List &array() const { return m_array; }
    TQValueList<TQString> keys() const;
    bool contains(const TQString &key) const { return m_object.find(key) != m_object.end(); }
    SdJson operator[](const TQString &key) const;
    TQString str(const TQString &key) const { return (*this)[key].toString(); }

private:
    friend class SdJsonParser;
    Type m_type;
    bool m_bool;
    TQString m_text;
    std::vector<SdJson> m_array;
    std::map<TQString, SdJson> m_object;
};

#endif
