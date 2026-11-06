/*
 * Copyright (C) 2026 tde-systemd developers
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "sdjson.h"

#include <tqstringlist.h>

#include <ctype.h>
#include <string.h>

class SdJsonParser
{
public:
    SdJsonParser(const TQCString &t) : s(t.data()), n(t.length()), i(0), ok(true) {}

    SdJson value()
    {
        SdJson v;
        ws();
        if (i >= n) {
            ok = false;
            return v;
        }
        char c = s[i];
        if (c == '{') {
            v.m_type = SdJson::Object;
            ++i;
            ws();
            if (peek() == '}') {
                ++i;
                return v;
            }
            while (ok) {
                ws();
                TQString key = string();
                ws();
                if (peek() != ':') {
                    ok = false;
                    break;
                }
                ++i;
                v.m_object[key] = value();
                ws();
                if (peek() == ',') {
                    ++i;
                    continue;
                }
                if (peek() == '}') {
                    ++i;
                    break;
                }
                ok = false;
            }
        } else if (c == '[') {
            v.m_type = SdJson::Array;
            ++i;
            ws();
            if (peek() == ']') {
                ++i;
                return v;
            }
            while (ok) {
                v.m_array.push_back(value());
                ws();
                if (peek() == ',') {
                    ++i;
                    continue;
                }
                if (peek() == ']') {
                    ++i;
                    break;
                }
                ok = false;
            }
        } else if (c == '"') {
            v.m_type = SdJson::String;
            v.m_text = string();
        } else if (literal("true")) {
            v.m_type = SdJson::Bool;
            v.m_bool = true;
        } else if (literal("false")) {
            v.m_type = SdJson::Bool;
        } else if (literal("null")) {
            v.m_type = SdJson::Null;
        } else {
            uint start = i;
            while (i < n && (isdigit((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' ||
                             s[i] == '.' || s[i] == 'e' || s[i] == 'E'))
                ++i;
            if (i == start) {
                ok = false;
                return v;
            }
            v.m_type = SdJson::Number;
            v.m_text = TQString::fromLatin1(s + start, i - start);
        }
        return v;
    }

    void ws()
    {
        while (i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r'))
            ++i;
    }
    char peek() const { return i < n ? s[i] : 0; }

    bool literal(const char *lit)
    {
        uint l = strlen(lit);
        if (i + l <= n && strncmp(s + i, lit, l) == 0) {
            i += l;
            return true;
        }
        return false;
    }

    static int hex(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    uint hex4()
    {
        if (i + 4 > n) {
            ok = false;
            return 0;
        }
        uint v = 0;
        for (int k = 0; k < 4; ++k) {
            int h = hex(s[i + k]);
            if (h < 0) {
                ok = false;
                return 0;
            }
            v = v * 16 + h;
        }
        i += 4;
        return v;
    }

    static void appendUtf8(TQCString &out, uint cp)
    {
        char b[5] = { 0, 0, 0, 0, 0 };
        if (cp < 0x80) {
            b[0] = cp;
        } else if (cp < 0x800) {
            b[0] = 0xc0 | (cp >> 6);
            b[1] = 0x80 | (cp & 0x3f);
        } else if (cp < 0x10000) {
            b[0] = 0xe0 | (cp >> 12);
            b[1] = 0x80 | ((cp >> 6) & 0x3f);
            b[2] = 0x80 | (cp & 0x3f);
        } else {
            b[0] = 0xf0 | (cp >> 18);
            b[1] = 0x80 | ((cp >> 12) & 0x3f);
            b[2] = 0x80 | ((cp >> 6) & 0x3f);
            b[3] = 0x80 | (cp & 0x3f);
        }
        out += b;
    }

    TQString string()
    {
        if (peek() != '"') {
            ok = false;
            return TQString::null;
        }
        ++i;
        TQCString out;
        uint start = i;
        while (i < n && s[i] != '"') {
            if (s[i] == '\\') {
                if (i > start)
                    out += TQCString(s + start, i - start + 1);
                ++i;
                if (i >= n)
                    break;
                char e = s[i++];
                switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    uint cp = hex4();
                    if (cp >= 0xd800 && cp < 0xdc00 && i + 6 <= n && s[i] == '\\' && s[i + 1] == 'u') {
                        i += 2;
                        uint lo = hex4();
                        cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: out += e; break;
                }
                start = i;
            } else {
                ++i;
            }
        }
        if (i > start)
            out += TQCString(s + start, i - start + 1);
        if (i >= n)
            ok = false;
        else
            ++i;  // closing quote
        return TQString::fromUtf8(out);
    }

    const char *s;
    uint n;
    uint i;
    bool ok;
};

SdJson SdJson::parse(const TQCString &text, bool *ok)
{
    SdJsonParser p(text);
    SdJson v = p.value();
    if (ok)
        *ok = p.ok;
    return v;
}

TQString SdJson::toString() const
{
    switch (m_type) {
    case String:
    case Number:
        return m_text;
    case Bool:
        return m_bool ? "true" : "false";
    case Array: {
        // journald encodes non UTF-8 fields as arrays of bytes
        bool bytes = !m_array.empty();
        for (List::const_iterator it = m_array.begin(); it != m_array.end(); ++it) {
            if ((*it).m_type != Number) {
                bytes = false;
                break;
            }
        }
        if (bytes) {
            TQCString raw;
            for (List::const_iterator it = m_array.begin(); it != m_array.end(); ++it) {
                char c = (char)(*it).m_text.toInt();
                if (c == 0)
                    c = ' ';
                raw += c;
            }
            return TQString::fromUtf8(raw);
        }
        TQStringList parts;
        for (List::const_iterator it = m_array.begin(); it != m_array.end(); ++it)
            parts << (*it).toString();
        return parts.join(", ");
    }
    default:
        return TQString::null;
    }
}

SdJson SdJson::operator[](const TQString &key) const
{
    std::map<TQString, SdJson>::const_iterator it = m_object.find(key);
    if (it == m_object.end())
        return SdJson();
    return it->second;
}

TQValueList<TQString> SdJson::keys() const
{
    TQValueList<TQString> k;
    for (std::map<TQString, SdJson>::const_iterator it = m_object.begin(); it != m_object.end(); ++it)
        k.append(it->first);
    return k;
}
