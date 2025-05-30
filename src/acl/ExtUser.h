/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_EXTUSER_H
#define SQUID_SRC_ACL_EXTUSER_H

#if USE_AUTH

#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "acl/Data.h"

class ACLExtUser : public Acl::Node
{
    MEMPROXY_CLASS(ACLExtUser);

public:
    ACLExtUser(ACLData<char const *> *newData, char const *);
    ~ACLExtUser() override;

    /* Acl::Node API */
    char const *typeString() const override;
    void parse() override;
    int match(ACLChecklist *checklist) override;
    bool requiresRequest() const override { return true; }
    SBufList dump() const override;
    bool empty () const override;

private:
    /* Acl::Node API */
    const Acl::Options &lineOptions() override;

    ACLData<char const *> *data;
    char const *type_;
};

#endif /* USE_AUTH */
#endif /* SQUID_SRC_ACL_EXTUSER_H */

