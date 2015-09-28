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

#include <cassert>
#include <cerrno>

#include <rpc.h>

static void throw_uuid_error(const char *context, RPC_STATUS rc)
{
    switch (rc) {
    case RPC_S_UUID_LOCAL_ONLY:
        errno = EIO;
        break;
    case RPC_S_UUID_NO_ADDRESS:
        errno = ENXIO;
        break;
    case RPC_S_OUT_OF_MEMORY:
        errno = ENOMEM;
        break;
    default:
        errno = EINVAL;
        break;
    }
    throw_posix_error(context);
}

static void uuid_generate_random(uuid_t &uu)
{
    RPC_STATUS rc;
    rc = UuidCreate(&uu);
    if (rc != RPC_S_OK)
        throw_uuid_error("UuidCreate()", rc);
}

static void uuid_unparse_lower(uuid_t &uu, char *out)
{
    unsigned char *us;
    RPC_STATUS rc;
    rc = UuidToString(&uu, &us);
    if (rc != RPC_S_OK)
        throw_uuid_error("UuidToString()", rc);
    const char *s = reinterpret_cast<const char *>(us);
    assert(strlen(s) == 36U);
    strcpy(out, s);
    RpcStringFree(&us);
}

#elif HAVE_DCE_UUID

#include "system.hh"

#include <cassert>
#include <cerrno>

#include <stdint.h>
#include <uuid.h>

static void throw_uuid_error(const char *context, uint32_t rc)
{
    errno = (rc == uuid_s_no_memory) ? ENOMEM : EINVAL;
    throw_posix_error(context);
}

static void uuid_generate_random(uuid_t &uu)
{
    uint32_t rc;
    uuid_create(&uu, &rc);
    if (rc != uuid_s_ok)
        throw_uuid_error("uuid_create()", rc);
}

static void uuid_unparse_lower(uuid_t &uu, char *out)
{
    uint32_t rc;
    char *s;
    uuid_to_string(&uu, &s, &rc);
    if (rc != uuid_s_ok)
        throw_uuid_error("uuid_to_string()", rc);
    assert(strlen(s) == 36U);
    strcpy(out, s);
    free(s);
}

#else

#include <uuid.h>

#endif

#endif

// vim:ts=4 sts=4 sw=4 et
