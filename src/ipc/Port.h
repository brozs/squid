/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 54    Interprocess Communication */

#ifndef SQUID_SRC_IPC_PORT_H
#define SQUID_SRC_IPC_PORT_H

#include "ipc/UdsOp.h"
#include "SquidString.h"

namespace Ipc
{

/// Waits for and receives incoming IPC messages; kids handle the messages
class Port: public UdsOp
{
public:
    Port(const String &aListenAddr);
    /// calculates IPC message address for strand #id of processLabel type
    static String MakeAddr(const char *proccessLabel, int id);

    /// get the IPC message address for coordinator process
    static String CoordinatorAddr();

protected:
    void start() override = 0; // UdsOp (AsyncJob) API; has body
    bool doneAll() const override; // UdsOp (AsyncJob) API

    /// read the next incoming message
    void doListen();

    /// handle IPC message just read
    /// kids must call parent method when they do not recognize the message type
    virtual void receive(const TypedMsgHdr &) = 0;

private:
    void noteRead(const CommIoCbParams &params); // Comm callback API
    void receiveOrIgnore(const TypedMsgHdr& );

private:
    TypedMsgHdr buf; ///< msghdr struct filled by Comm
};

extern const char strandAddrLabel[]; ///< strand's listening address unique label

} // namespace Ipc

#endif /* SQUID_SRC_IPC_PORT_H */

