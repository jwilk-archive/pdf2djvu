/* Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdfdjvu.
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

#ifndef PDF2DJVU_WIN32_UUID_H
#define PDF2DJVU_WIN32_UUID_H

#include <stdexcept>

#include <rpc.h>

#include "i18n.hh"

class UuidOperationFailure : public std::runtime_error
{
public:
    explicit UuidOperationFailure(const std::string &message)
    : std::runtime_error(message)
    { };
};

static void uuid_generate_random(uuid_t &uu)
{
    RPC_STATUS rc;
    rc = UuidCreate(&uu);
    if (rc != RPC_S_OK)
        throw UuidOperationFailure(_("UuidCreate() failed"));
}

static void uuid_unparse_lower(uuid_t &uu, char *out)
{
    unsigned char *us;
    RPC_STATUS rc;
    rc = UuidToString(&uu, &us);
    if (rc != RPC_S_OK)
        throw UuidOperationFailure(_("UuidToString() failed"));
    const char *s = reinterpret_cast<const char *>(us);
    if (strlen(s) != 36U) {
        RpcStringFree(&us);
        throw UuidOperationFailure(_("UuidToString(): unexpected string length"));
    }
    strcpy(out, s);
    RpcStringFree(&us);
}

#endif

// vim:ts=4 sts=4 sw=4 et
