/* Copyright © 2009-2022 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdf2djvu.
 *
 * pdf2djvu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * pdf2djvu is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "xmp.hh"

#include "autoconf.hh"
#include "i18n.hh"

#if HAVE_XMP

#include <cassert>
#include <climits>
#include <vector>

#include <exiv2/exiv2.hpp>

#include "debug.hh"
#include "string-printf.hh"
#include "sys-uuid.hh"
#include "version.hh"

static void maybe_set(Exiv2::XmpData &data, const char *key, const std::string &value)
{
    if (value.length() == 0)
        return;
    Exiv2::XmpData::iterator it = data.findKey(Exiv2::XmpKey(key));
    if (it == data.end())
        data[key] = value;
}

static std::string maybe_get(Exiv2::XmpData &data, const char *key)
{
    Exiv2::XmpData::iterator it = data.findKey(Exiv2::XmpKey(key));
    if (it == data.end())
        return "";
    return it->toString();
}

static void set_history(Exiv2::XmpData &data, long n, const char *event, const std::string &value)
{
    const std::string key = string_printf("Xmp.xmpMM.History[%ld]/stEvt:%s", n, event);
    data[key] = value;
}

static void error_handler(int level, const char *message)
{
    const char *category = (level == Exiv2::LogMsg::error)
        ? _("XMP metadata error")
        : _("XMP metadata warning");
    error_log <<
      /* L10N: "<error-category>: <error-message>" */
      string_printf(_("%s: %s"), category, message);
}

static std::string gen_uuid()
{
    char uuid_s[36 + 1];
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, uuid_s);
    return std::string("urn:uuid:") + uuid_s;
}

std::string xmp::transform(const std::string &ibytes, const pdf::Metadata &metadata)
{
    Exiv2::LogMsg::setHandler(error_handler);
    Exiv2::XmpData data;
    int rc;
    rc = Exiv2::XmpParser::decode(data, ibytes);
    if (rc != 0)
        throw xmp::Error(_("Unable to parse XMP metadata"));
    std::string instance_id = gen_uuid();
    std::string document_id = gen_uuid();
    std::string original_document_id;
    std::string obytes;
    maybe_set(data, "Xmp.dc.title", metadata.title);
    maybe_set(data, "Xmp.dc.creator", metadata.author);
    maybe_set(data, "Xmp.dc.description", metadata.subject);
    data["Xmp.dc.format"] = "image/vnd.djvu";
    maybe_set(data, "Xmp.pdf.Keywords", metadata.keywords);
    maybe_set(data, "Xmp.pdf.Producer", metadata.producer);
    maybe_set(data, "Xmp.xmp.CreatorTool", metadata.creator);
    try {
        std::string date = metadata.creation_date.format('T');
        maybe_set(data, "Xmp.xmp.CreateDate", date);
    } catch (const pdf::Timestamp::Invalid &) {
        /* Ignore the error. User should be warned elsewhere anyway. */
    }
    try {
        std::string date = metadata.mod_date.format('T');
        maybe_set(data, "Xmp.xmp.ModifyDate", date);
    } catch (const pdf::Timestamp::Invalid &) {
        /* Ignore the error. User should be warned elsewhere anyway. */
    }
    std::string now_date = pdf::Timestamp::now().format('T');
    data["Xmp.xmp.MetadataDate"] = now_date;
    if (data.findKey(Exiv2::XmpKey("Xmp.xmpMM.History")) == data.end()) {
        auto empty_seq = Exiv2::Value::create(Exiv2::xmpSeq);
        data.add(Exiv2::XmpKey("Xmp.xmpMM.History"), empty_seq.get());
    }
    data["Xmp.xmpMM.InstanceID"] = instance_id;
    original_document_id = maybe_get(data, "Xmp.xmpMM.OriginalDocumentID");
    if (original_document_id.length() == 0)
        original_document_id = maybe_get(data, "Xmp.xmpMM.DocumentID");
    if (original_document_id.length() > 0)
        data["Xmp.xmpMM.OriginalDocumentID"] = original_document_id;
    data["Xmp.xmpMM.DocumentID"] = document_id;
    {
        long n = data["Xmp.xmpMM.History"].count();
        assert((n >= 0) && (n < LONG_MAX));
        n++;
        set_history(data, n, "action", "converted");
        set_history(data, n, "parameters", "from application/pdf to image/vnd.djvu");
        set_history(data, n, "instanceID", instance_id);
        set_history(data, n, "softwareAgent", get_version());
        set_history(data, n, "when", now_date);
    }
    Exiv2::XmpParser::encode(obytes, data, Exiv2::XmpParser::omitPacketWrapper);
    return obytes;
}

#else

std::string xmp::transform(const std::string &data, const pdf::Metadata &metadata)
{
    throw xmp::Error(_("pdf2djvu was built without support for updating XMP."));
}

#endif

// vim:ts=4 sts=4 sw=4 et
