/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_IP_QOSCONFIG_H
#define SQUID_SRC_IP_QOSCONFIG_H

#include "acl/forward.h"
#include "cbdata.h"
#include "comm/forward.h"
#include "hier_code.h"
#include "ip/forward.h"
#include "ip/NfMarkConfig.h"
#include "store/forward.h"

#if HAVE_LIBNETFILTER_CONNTRACK_LIBNETFILTER_CONNTRACK_H
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#endif
#if HAVE_LIBNETFILTER_CONNTRACK_LIBNETFILTER_CONNTRACK_TCP_H
#include <libnetfilter_conntrack/libnetfilter_conntrack_tcp.h>
#endif
#include <iosfwd>
#include <limits>

class fde;

// TODO: move to new Acl::Node framework
class acl_tos
{
    CBDATA_CLASS(acl_tos);

public:
    acl_tos() : next(nullptr), aclList(nullptr), tos(0) {}
    ~acl_tos();

    acl_tos *next;
    ACLList *aclList;
    tos_t tos;
};

// TODO: move to new Acl::Node framework
class acl_nfmark
{
    CBDATA_CLASS(acl_nfmark);

public:
    acl_nfmark() : next(nullptr), aclList(nullptr) {}
    ~acl_nfmark();

    acl_nfmark *next;
    ACLList *aclList;
    Ip::NfMarkConfig markConfig;
};

namespace Ip
{

/**
 * QOS namespace contains all the QOS functionality: global functions within
 * the namespace and the configuration parameters within a config class.
 */
namespace Qos
{

/// Possible Squid roles in connection handling
enum ConnectionDirection {
    dirAccepted, ///< accepted (from a client by Squid)
    dirOpened ///< opened (by Squid to an origin server or peer)
};

/**
* Function to retrieve the TOS value of the inbound packet.
* Called by FwdState::dispatch if QOS options are enabled.
* Bug 2537: This part of ZPH only applies to patched Linux kernels
* @param server    Server side descriptor of connection to get TOS for
* @param clientFde Pointer to client side fde instance to set tosFromServer in
*/
void getTosFromServer(const Comm::ConnectionPointer &server, fde *clientFde);

/**
* Function to retrieve the netfilter CONNMARK value of the connection.
* Called by FwdState::dispatch if QOS options are enabled or by
* Comm::TcpAcceptor::acceptOne
*
* @param conn    Pointer to connection to get mark for
* @param connDir Specifies connection type (incoming or outgoing)
*/
nfmark_t getNfConnmark(const Comm::ConnectionPointer &conn, const ConnectionDirection connDir);

/**
* Function to set the netfilter CONNMARK value on the connection.
* Called by ClientHttpRequest::doCallouts.
*
* @param conn    Pointer to connection to set mark on
* @param connDir Specifies connection type (incoming or outgoing)
* @cm            Netfilter mark configuration (mark and mask)
*/
bool setNfConnmark(Comm::ConnectionPointer &conn, const ConnectionDirection connDir, const NfMarkConfig &cm);

/**
* Function to work out and then apply to the socket the appropriate
* TOS value to set on packets when items have not been retrieved from
* local cache. Called by clientReplyContext::sendMoreData if QOS is
* enabled for TOS.
* @param conn     Descriptor of socket to set the TOS for
* @param hierCode Hier code of request
*/
int doTosLocalMiss(const Comm::ConnectionPointer &conn, const hier_code hierCode);

/**
* Function to work out and then apply to the socket the appropriate
* netfilter mark value to set on packets when items have not been
* retrieved from local cache. Called by clientReplyContext::sendMoreData
* if QOS is enabled for TOS.
* @param conn     Descriptor of socket to set the mark for
* @param hierCode Hier code of request
*/
int doNfmarkLocalMiss(const Comm::ConnectionPointer &conn, const hier_code hierCode);

/**
* Function to work out and then apply to the socket the appropriate
* TOS value to set on packets when items *have* been retrieved from
* local cache. Called by clientReplyContext::doGetMoreData if QOS is
* enabled for TOS.
* @param conn Descriptor of socket to set the TOS for
*/
int doTosLocalHit(const Comm::ConnectionPointer &conn);

/**
* Function to work out and then apply to the socket the appropriate
* netfilter mark value to set on packets when items *have* been
* retrieved from local cache. Called by clientReplyContext::doGetMoreData
* if QOS is enabled for TOS.
* @param conn Descriptor of socket to set the mark for
*/
int doNfmarkLocalHit(const Comm::ConnectionPointer &conn);

/**
* Function to set the TOS value of packets. Sets the value on the socket
* which then gets copied to the packets.
* @param conn Descriptor of socket to set the TOS for
*/
int setSockTos(const Comm::ConnectionPointer &conn, tos_t tos);

/**
* The low level variant of setSockTos function to set TOS value of packets.
* Avoid if you can use the Connection-based setSockTos().
* @param fd Descriptor of socket to set the TOS for
* @param type The socket family, AF_INET or AF_INET6
*/
int setSockTos(const int fd, tos_t tos, int type);

/**
* Function to set the netfilter mark value of packets. Sets the value on the
* socket which then gets copied to the packets. Called from Ip::Qos::doNfmarkLocalMiss
* @param conn Descriptor of socket to set the mark for
*/
int setSockNfmark(const Comm::ConnectionPointer &conn, nfmark_t mark);

/**
* The low level variant of setSockNfmark function to set the netfilter mark
* value of packets.
* Avoid if you can use the Connection-based setSockNfmark().
* @param fd Descriptor of socket to set the mark for
*/
int setSockNfmark(const int fd, nfmark_t mark);

/**
 * QOS configuration class. Contains all the parameters for QOS functions as well
 * as functions to check whether either TOS or MARK QOS is enabled.
 */
class Config
{
public:

    Config();
    ~Config() {}

    void parseConfigLine();

    /**
     * Dump all the configuration values
     *
     * NOTE: Due to the low-level nature of the library these
     * objects are part of the dump function must be self-contained.
     * which means no StoreEntry references. Just a basic char* buffer.
     */
    void dumpConfigLine(std::ostream &, const char *) const;

    /// Whether we should modify TOS flags based on cache hits and misses.
    bool isHitTosActive() const {
        return (tosLocalHit || tosSiblingHit || tosParentHit || tosMiss || preserveMissTos);
    }

    /// Whether we should modify netfilter marks based on cache hits and misses.
    bool isHitNfmarkActive() const {
        return (markLocalHit || markSiblingHit || markParentHit || markMiss || preserveMissMark);
    }

    /**
    * Iterates through any outgoing_nfmark or clientside_nfmark configuration parameters
    * to find out if any Netfilter marking is required.
    * This function is used on initialisation to define capabilities required (Netfilter
    * marking requires CAP_NET_ADMIN).
    */
    bool isAclNfmarkActive() const;

    /**
    * Iterates through any outgoing_tos or clientside_tos configuration parameters
    * to find out if packets should be marked with TOS flags.
    */
    bool isAclTosActive() const;

    tos_t tosLocalHit;                  ///< TOS value to apply to local cache hits
    tos_t tosSiblingHit;                ///< TOS value to apply to hits from siblings
    tos_t tosParentHit;                 ///< TOS value to apply to hits from parent
    tos_t tosMiss;                      ///< TOS value to apply to cache misses
    tos_t tosMissMask;                  ///< Mask for TOS value to apply to cache misses. Applied to the tosMiss value.
    bool preserveMissTos;               ///< Whether to preserve the TOS value of the inbound packet for misses
    tos_t preserveMissTosMask;          ///< The mask to apply when preserving the TOS of misses. Applies to preserved value from upstream.

    nfmark_t markLocalHit;              ///< Netfilter mark value to apply to local cache hits
    nfmark_t markSiblingHit;            ///< Netfilter mark value to apply to hits from siblings
    nfmark_t markParentHit;             ///< Netfilter mark value to apply to hits from parent
    nfmark_t markMiss;                  ///< Netfilter mark value to apply to cache misses
    nfmark_t markMissMask;              ///< Mask for netfilter mark value to apply to cache misses. Applied to the markMiss value.
    bool preserveMissMark;              ///< Whether to preserve netfilter mark value of inbound connection
    nfmark_t preserveMissMarkMask;      ///< The mask to apply when preserving the netfilter mark of misses. Applied to preserved value from upstream.

    acl_tos *tosToServer;               ///< The TOS that packets to the web server should be marked with, based on ACL
    acl_tos *tosToClient;               ///< The TOS that packets to the client should be marked with, based on ACL
    acl_nfmark *nfmarkToServer;         ///< The MARK that packets to the web server should be marked with, based on ACL
    acl_nfmark *nfmarkToClient;         ///< The MARK that packets to the client should be marked with, based on ACL
    acl_nfmark *nfConnmarkToClient = nullptr;    ///< The CONNMARK that the client connection should be marked with, based on ACL

};

/// Globally available instance of Qos::Config
extern Config TheConfig;

} // namespace Qos

} // namespace Ip

/* legacy parser access wrappers */
inline void parse_QosConfig(Ip::Qos::Config * c) { c->parseConfigLine(); }
inline void free_QosConfig(Ip::Qos::Config *) {}
void dump_QosConfig(StoreEntry *, const char * directiveName, const Ip::Qos::Config &);

#endif /* SQUID_SRC_IP_QOSCONFIG_H */

