/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 28    Access Control */

#include "squid.h"
#include "acl/Time.h"
#include "time/gadgets.h"

int
Acl::CurrentTimeCheck::match(ACLChecklist *)
{
    return data->match(squid_curtime);
}

