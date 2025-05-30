/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_HTTPHEADERMASK_H
#define SQUID_SRC_HTTPHEADERMASK_H

/* big mask for http headers */
typedef char HttpHeaderMask[12];

void httpHeaderMaskInit(HttpHeaderMask * mask, int value);

#endif /* SQUID_SRC_HTTPHEADERMASK_H */

