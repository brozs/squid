/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 16    Cache Manager API */

#include "squid.h"
#include "AccessLogEntry.h"
#include "base/AsyncJobCalls.h"
#include "base/TextException.h"
#include "comm.h"
#include "comm/Connection.h"
#include "CommCalls.h"
#include "errorpage.h"
#include "globals.h"
#include "HttpReply.h"
#include "HttpRequest.h"
#include "ipc/Port.h"
#include "mgr/Forwarder.h"
#include "mgr/Request.h"
#include "Store.h"

CBDATA_NAMESPACED_CLASS_INIT(Mgr, Forwarder);

Mgr::Forwarder::Forwarder(const Comm::ConnectionPointer &aConn, const ActionParams &aParams,
                          HttpRequest* aRequest, StoreEntry* anEntry, const AccessLogEntryPointer &anAle):
    // TODO: Add virtual Forwarder::makeRequest() to avoid prematurely creating
    // this dummy request with a dummy ID that are finalized by Ipc::Forwarder.
    // Same for Snmp::Forwarder.
    Ipc::Forwarder(new Request(KidIdentifier, Ipc::RequestId(/*XXX*/), aConn, aParams), 10),
    httpRequest(aRequest), entry(anEntry), conn(aConn), ale(anAle)
{
    debugs(16, 5, conn);
    Must(Comm::IsConnOpen(conn));
    Must(httpRequest != nullptr);
    Must(entry != nullptr);

    HTTPMSGLOCK(httpRequest);
    entry->lock("Mgr::Forwarder");

    closer = asyncCall(16, 5, "Mgr::Forwarder::noteCommClosed",
                       CommCbMemFunT<Forwarder, CommCloseCbParams>(this, &Forwarder::noteCommClosed));
    comm_add_close_handler(conn->fd, closer);
}

Mgr::Forwarder::~Forwarder()
{
    SWALLOW_EXCEPTIONS({
        Must(entry);
        entry->unlock("Mgr::Forwarder");
        Must(httpRequest);
        HTTPMSGUNLOCK(httpRequest);
    });
}

/// closes our copy of the client HTTP connection socket
void
Mgr::Forwarder::swanSong()
{
    if (Comm::IsConnOpen(conn)) {
        if (closer != nullptr) {
            comm_remove_close_handler(conn->fd, closer);
            closer = nullptr;
        }
        conn->close();
    }
    conn = nullptr;
    Ipc::Forwarder::swanSong();
}

void
Mgr::Forwarder::handleError()
{
    debugs(16, DBG_CRITICAL, "ERROR: uri " << entry->url() << " exceeds buffer size");
    sendError(new ErrorState(ERR_INVALID_URL, Http::scUriTooLong, httpRequest, ale));
    mustStop("long URI");
}

void
Mgr::Forwarder::handleTimeout()
{
    sendError(new ErrorState(ERR_LIFETIME_EXP, Http::scRequestTimeout, httpRequest, ale));
    Ipc::Forwarder::handleTimeout();
}

void
Mgr::Forwarder::handleException(const std::exception &e)
{
    if (entry != nullptr && httpRequest != nullptr && Comm::IsConnOpen(conn))
        sendError(new ErrorState(ERR_INVALID_RESP, Http::scInternalServerError, httpRequest, ale));
    Ipc::Forwarder::handleException(e);
}

/// called when the client socket gets closed by some external force
void
Mgr::Forwarder::noteCommClosed(const CommCloseCbParams &)
{
    debugs(16, 5, MYNAME);
    closer = nullptr;
    if (conn) {
        conn->noteClosure();
        conn = nullptr;
    }
    mustStop("commClosed");
}

/// send error page
void
Mgr::Forwarder::sendError(ErrorState *error)
{
    debugs(16, 3, MYNAME);
    Must(error != nullptr);
    Must(entry != nullptr);
    Must(httpRequest != nullptr);

    entry->buffer();
    entry->replaceHttpReply(error->BuildHttpReply());
    entry->expires = squid_curtime;
    delete error;
    entry->flush();
    entry->complete();
}

