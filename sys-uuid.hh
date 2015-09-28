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

#ifndef PDF2DJVU_SYS_UUID_H
#define PDF2DJVU_SYS_UUID_H

#include "autoconf.hh"

#if WIN32

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

#elif HAVE_DCE_UUID

#include "system.hh"

#include <cassert>
#include <cerrno>

#include <stdint.h>
#include <uuid.h>

static void throw_uuid_error(uint32_t rc)
{
    errno = (rc == uuid_s_no_memory) ? ENOMEM : EINVAL;
    throw_posix_error("uuid_to_string()");
}

static void uuid_generate_random(uuid_t &uu)
{
    uint32_t rc;
    uuid_create(&uu, &rc);
    if (rc != uuid_s_ok)
        throw_uuid_error(rc);
}

static void uuid_unparse_lower(uuid_t &uu, char *out)
{
    uint32_t rc;
    char *s;
    uuid_to_string(&uu, &s, &rc);
    if (rc != uuid_s_ok)
        throw_uuid_error(rc);
    assert(strlen(s) == 36U);
    strcpy(out, s);
    free(s);
}

#else

#include <uuid.h>

#endif

#endif

// vim:ts=4 sts=4 sw=4 et
