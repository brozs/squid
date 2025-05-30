/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_SERVERS_HTTP1SERVER_H
#define SQUID_SRC_SERVERS_HTTP1SERVER_H

#include "client_side.h"
#include "http/one/RequestParser.h"
#include "http/Stream.h"
#include "servers/forward.h"

namespace Http
{
namespace One
{

/// Manages a connection from an HTTP/1 or HTTP/0.9 client.
class Server: public ConnStateData
{
    CBDATA_CHILD(Server);

public:
    Server(const MasterXaction::Pointer &xact, const bool beHttpsServer);
    ~Server() override {}

    void readSomeHttpData();

protected:
    /* ConnStateData API */
    Http::Stream *parseOneRequest() override;
    void processParsedRequest(Http::StreamPointer &context) override;
    void handleReply(HttpReply *rep, StoreIOBuffer receivedData) override;
    bool writeControlMsgAndCall(HttpReply *rep, AsyncCall::Pointer &call) override;
    int pipelinePrefetchMax() const override;
    time_t idleTimeout() const override;
    void noteTakeServerConnectionControl(ServerConnectionContext) override;

    /* BodyPipe API */
    void noteMoreBodySpaceAvailable(BodyPipe::Pointer) override;
    void noteBodyConsumerAborted(BodyPipe::Pointer) override;

    /* AsyncJob API */
    void start() override;

    void proceedAfterBodyContinuation(Http::StreamPointer context);

private:
    void processHttpRequest(Http::Stream *const context);
    void handleHttpRequestData();

    /// Handles parsing results. May generate and deliver an error reply
    /// to the client if parsing is failed, or parses the url and build the
    /// HttpRequest object using parsing results.
    /// Return false if parsing is failed, true otherwise.
    bool buildHttpRequest(Http::StreamPointer &context);

    void setReplyError(Http::StreamPointer &context, HttpRequest::Pointer &request, err_type requestError, Http::StatusCode errStatusCode, const char *requestErrorBytes);

    Http1::RequestParserPointer parser_;
    HttpRequestMethod method_; ///< parsed HTTP method

    /// temporary hack to avoid creating a true HttpsServer class
    const bool isHttpsServer;
};

} // namespace One
} // namespace Http

#endif /* SQUID_SRC_SERVERS_HTTP1SERVER_H */

