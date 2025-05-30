/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_NEIGHBORTYPEDOMAINLIST_H
#define SQUID_SRC_NEIGHBORTYPEDOMAINLIST_H

#include "enums.h"

/// representation of a neighbor_type_domain configuration directive. A POD
class NeighborTypeDomainList
{
public:
    char *domain;
    peer_t type;
    NeighborTypeDomainList *next;
};

#endif /* SQUID_SRC_NEIGHBORTYPEDOMAINLIST_H */

