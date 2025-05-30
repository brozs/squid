/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_INCLUDE_SQUID_H
#define SQUID_INCLUDE_SQUID_H

#include "autoconf.h"       /* For GNU autoconf variables */
#include "version.h"

/* default values for listen ports. Usually specified in squid.conf really */
#define CACHE_HTTP_PORT 3128
#define CACHE_ICP_PORT 3130

/* To keep API definitions clear */
#ifdef __cplusplus
#define SQUIDCEXTERN extern "C"
#else
#define SQUIDCEXTERN extern
#endif

/****************************************************************************
 *--------------------------------------------------------------------------*
 * DO *NOT* MAKE ANY CHANGES below here unless you know what you're doing...*
 *--------------------------------------------------------------------------*
 ****************************************************************************/

#include "compat/compat.h"

#ifdef USE_POSIX_REGEX
#ifndef USE_RE_SYNTAX
#define USE_RE_SYNTAX   REG_EXTENDED    /* default Syntax */
#endif
#endif

#if SQUID_DETECT_UDP_SO_SNDBUF > 16384
#define SQUID_UDP_SO_SNDBUF 16384
#else
#define SQUID_UDP_SO_SNDBUF SQUID_DETECT_UDP_SO_SNDBUF
#endif

#if SQUID_DETECT_UDP_SO_RCVBUF > 16384
#define SQUID_UDP_SO_RCVBUF 16384
#else
#define SQUID_UDP_SO_RCVBUF SQUID_DETECT_UDP_SO_RCVBUF
#endif

/* temp hack: needs to be pre-defined for now. */
#define SQUID_MAXPATHLEN 256

// TODO: determine if this is required. OR if compat/os/mswindows.h works
#if _SQUID_WINDOWS_ && defined(__cplusplus)
/** \cond AUTODOCS-IGNORE */
using namespace Squid;
/** \endcond */
#endif

#define LOCAL_ARRAY(type, name, size) static type name[size]

#endif /* SQUID_INCLUDE_SQUID_H */

