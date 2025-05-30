/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 33    Client-side Routines */

#ifndef SQUID_SRC_SERVERS_FTPSERVER_H
#define SQUID_SRC_SERVERS_FTPSERVER_H

#include "base/JobWait.h"
#include "base/Lock.h"
#include "client_side.h"
#include "comm/forward.h"
#include "http/forward.h"

namespace Ftp
{

typedef enum {
    fssBegin,
    fssConnected,
    fssHandleFeat,
    fssHandlePasv,
    fssHandlePort,
    fssHandleDataRequest,
    fssHandleUploadRequest,
    fssHandleEprt,
    fssHandleEpsv,
    fssHandleCwd,
    fssHandlePass,
    fssHandleCdup,
    fssError
} ServerState;

// TODO: This should become a part of MasterXaction when we start sending
// master transactions to the clients/ code.
/// Transaction information shared among our FTP client and server jobs.
class MasterState: public RefCountable
{
public:
    typedef RefCount<MasterState> Pointer;

    MasterState(): serverState(fssBegin), clientReadGreeting(false), userDataDone(0) {}

    Ip::Address clientDataAddr; ///< address of our FTP client data connection
    SBuf workingDir; ///< estimated current working directory for URI formation
    ServerState serverState; ///< what our FTP server is doing
    bool clientReadGreeting; ///< whether our FTP client read their FTP server greeting
    /// Squid will send or has sent this final status code to the FTP client
    int userDataDone;
};

/// Manages a control connection from an FTP client.
class Server: public ConnStateData
{
    CBDATA_CHILD(Server);

public:
    explicit Server(const MasterXaction::Pointer &xact);
    ~Server() override;

    /* AsyncJob API */
    void callException(const std::exception &e) override;

    /// Called by Ftp::Client class when it is start receiving or
    /// sending data.
    void startWaitingForOrigin();

    /// Called by Ftp::Client class when it is done receiving or
    /// sending data. Waits for both agents to be done before
    /// responding to the FTP client and closing the data connection.
    void stopWaitingForOrigin(int status);

    // This is a pointer in hope to minimize future changes when MasterState
    // becomes a part of MasterXaction. Guaranteed not to be nil.
    MasterState::Pointer master; ///< info shared among our FTP client and server jobs

protected:
    friend void StartListening();

    // errors detected before it is possible to create an HTTP request wrapper
    enum class EarlyErrorKind {
        HugeRequest,
        MissingLogin,
        MissingUsername,
        MissingHost,
        UnsupportedCommand,
        InvalidUri,
        MalformedCommand
    };

    /* ConnStateData API */
    Http::Stream *parseOneRequest() override;
    void processParsedRequest(Http::StreamPointer &context) override;
    void notePeerConnection(Comm::ConnectionPointer conn) override;
    void clientPinnedConnectionClosed(const CommCloseCbParams &io) override;
    void handleReply(HttpReply *header, StoreIOBuffer receivedData) override;
    int pipelinePrefetchMax() const override;
    bool writeControlMsgAndCall(HttpReply *rep, AsyncCall::Pointer &call) override;
    time_t idleTimeout() const override;

    /* BodyPipe API */
    void noteMoreBodySpaceAvailable(BodyPipe::Pointer) override;
    void noteBodyConsumerAborted(BodyPipe::Pointer ptr) override;

    /* AsyncJob API */
    void start() override;

    /* Comm callbacks */
    static void AcceptCtrlConnection(const CommAcceptCbParams &params);
    void acceptDataConnection(const CommAcceptCbParams &params);
    void readUploadData(const CommIoCbParams &io);
    void wroteEarlyReply(const CommIoCbParams &io);
    void wroteReply(const CommIoCbParams &io);
    void wroteReplyData(const CommIoCbParams &io);
    void connectedForData(const CommConnectCbParams &params);

    unsigned int listenForDataConnection();
    bool createDataConnection(Ip::Address cltAddr);
    void closeDataConnection();

    /// Called after data transfer on client-to-squid data connection is
    /// finished.
    void userDataCompletionCheckpoint(int finalStatusCode);

    /// Writes the data-transfer status reply to the FTP client and
    /// closes the data connection.
    void completeDataDownload();

    void calcUri(const SBuf *file);
    void changeState(const Ftp::ServerState newState, const char *reason);
    Http::Stream *handleUserRequest(const SBuf &cmd, SBuf &params);
    bool checkDataConnPost() const;
    void replyDataWritingCheckpoint();
    void maybeReadUploadData();

    void setReply(const int code, const char *msg);
    void writeCustomReply(const int code, const char *msg, const HttpReply *reply = nullptr);
    void writeEarlyReply(const int code, const char *msg);
    void writeErrorReply(const HttpReply *reply, const int status);
    void writeForwardedForeign(const HttpReply *reply);
    void writeForwardedReply(const HttpReply *reply);
    void writeForwardedReplyAndCall(const HttpReply *reply, AsyncCall::Pointer &call);
    void writeReply(MemBuf &mb);

    Http::Stream *earlyError(const EarlyErrorKind eek);
    bool handleRequest(HttpRequest *);
    void setDataCommand();
    bool checkDataConnPre();

    /// a method handling an FTP command; selected by handleRequest()
    typedef bool (Ftp::Server::*RequestHandler)(String &cmd, String &params);
    bool handleFeatRequest(String &cmd, String &params);
    bool handlePasvRequest(String &cmd, String &params);
    bool handlePortRequest(String &cmd, String &params);
    bool handleDataRequest(String &cmd, String &params);
    bool handleUploadRequest(String &cmd, String &params);
    bool handleEprtRequest(String &cmd, String &params);
    bool handleEpsvRequest(String &cmd, String &params);
    bool handleCwdRequest(String &cmd, String &params);
    bool handlePassRequest(String &cmd, String &params);
    bool handleCdupRequest(String &cmd, String &params);

    /// a method handling an FTP response; selected by handleReply()
    typedef void (Ftp::Server::*ReplyHandler)(const HttpReply *reply, StoreIOBuffer data);
    void handleFeatReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handlePasvReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handlePortReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handleErrorReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handleDataReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handleUploadReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handleEprtReply(const HttpReply *header, StoreIOBuffer receivedData);
    void handleEpsvReply(const HttpReply *header, StoreIOBuffer receivedData);

private:
    void doProcessRequest();
    void shovelUploadData();
    void resetLogin(const char *reason);

    SBuf uri; ///< a URI reconstructed from various FTP message details
    SBuf host; ///< intended dest. of a transparently intercepted FTP conn
    bool gotEpsvAll; ///< restrict data conn setup commands to just EPSV
    AsyncCall::Pointer onDataAcceptCall; ///< who to call upon data conn acceptance
    Comm::ConnectionPointer dataListenConn; ///< data connection listening socket
    Comm::ConnectionPointer dataConn; ///< data connection
    char uploadBuf[CLIENT_REQ_BUF_SZ]; ///< data connection input buffer
    size_t uploadAvailSize; ///< number of yet unused uploadBuf bytes

    AsyncCall::Pointer listener; ///< set when we are passively listening

    /// Waits for an FTP data connection to the client to be established/opened.
    /// This wait only happens in FTP active mode (via PORT or EPRT).
    JobWait<Comm::ConnOpener> dataConnWait;

    AsyncCall::Pointer reader; ///< set when we are reading FTP data

    /// whether we wait for the origin data transfer to end
    bool waitingForOrigin;
    /// whether the origin data transfer aborted
    bool originDataDownloadAbortedOnError;

    /// a response which writing was postponed until stopWaitingForOrigin()
    HttpReply::Pointer delayedReply;
};

} // namespace Ftp

#endif /* SQUID_SRC_SERVERS_FTPSERVER_H */

