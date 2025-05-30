/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 83    SSL accelerator support */

#ifndef SQUID_SRC_SSL_SUPPORT_H
#define SQUID_SRC_SSL_SUPPORT_H

#if USE_OPENSSL

#include "anyp/forward.h"
#include "base/CbDataList.h"
#include "base/TypeTraits.h"
#include "comm/forward.h"
#include "compat/openssl.h"
#include "dns/forward.h"
#include "ip/Address.h"
#include "sbuf/SBuf.h"
#include "security/Session.h"
#include "ssl/gadgets.h"

#if HAVE_OPENSSL_X509V3_H
#include <openssl/x509v3.h>
#endif
#if HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif
#if HAVE_OPENSSL_ENGINE_H
#include <openssl/engine.h>
#endif
#include <queue>
#include <map>
#include <optional>
#include <variant>

/**
 \defgroup ServerProtocolSSLAPI Server-Side SSL API
 \ingroup ServerProtocol
 */

// Maximum certificate validation callbacks. OpenSSL versions exceeding this
// limit are deemed stuck in an infinite validation loop (OpenSSL bug #3090)
// and will trigger the SQUID_X509_V_ERR_INFINITE_VALIDATION error.
// Can be set to a number up to UINT32_MAX
#ifndef SQUID_CERT_VALIDATION_ITERATION_MAX
#define SQUID_CERT_VALIDATION_ITERATION_MAX 16384
#endif

namespace AnyP
{
class PortCfg;
};

namespace Ipc
{
class MemMap;
}

namespace Ssl
{

/// callback for receiving password to access password secured PEM files
/// XXX: Requires SSL_CTX_set_default_passwd_cb_userdata()!
int AskPasswordCb(char *buf, int size, int rwflag, void *userdata);

/// initialize the SSL library global state.
/// call before generating any SSL context
void Initialize();

class CertValidationResponse;
typedef RefCount<CertValidationResponse> CertValidationResponsePointer;

/// initialize a TLS server context with OpenSSL specific settings
bool InitServerContext(Security::ContextPointer &, AnyP::PortCfg &);

/// initialize a TLS client context with OpenSSL specific settings
bool InitClientContext(Security::ContextPointer &, Security::PeerOptions &, Security::ParsedPortFlags);

/// set the certificate verify callback for a context
void ConfigurePeerVerification(Security::ContextPointer &, const Security::ParsedPortFlags);
void DisablePeerVerification(Security::ContextPointer &);

/// if required, setup callback for generating ephemeral RSA keys
void MaybeSetupRsaCallback(Security::ContextPointer &);

} //namespace Ssl

/// \ingroup ServerProtocolSSLAPI
const char *sslGetUserEmail(SSL *ssl);

/// \ingroup ServerProtocolSSLAPI
const char *sslGetUserAttribute(SSL *ssl, const char *attribute_name);

/// \ingroup ServerProtocolSSLAPI
const char *sslGetCAAttribute(SSL *ssl, const char *attribute_name);

/// \ingroup ServerProtocolSSLAPI
SBuf sslGetUserCertificatePEM(SSL *ssl);

/// \ingroup ServerProtocolSSLAPI
SBuf sslGetUserCertificateChainPEM(SSL *ssl);

namespace Ssl
{
/// \ingroup ServerProtocolSSLAPI
typedef char const *GETX509ATTRIBUTE(X509 *, const char *);
typedef SBuf GETX509PEM(X509 *);

/// \ingroup ServerProtocolSSLAPI
GETX509ATTRIBUTE GetX509UserAttribute;

/// \ingroup ServerProtocolSSLAPI
GETX509ATTRIBUTE GetX509CAAttribute;

/// \ingroup ServerProtocolSSLAPI
GETX509PEM GetX509PEM;

/// \ingroup ServerProtocolSSLAPI
GETX509ATTRIBUTE GetX509Fingerprint;

extern const EVP_MD *DefaultSignHash;

/**
  \ingroup ServerProtocolSSLAPI
 * Supported ssl-bump modes
 */
enum BumpMode {bumpNone = 0, bumpClientFirst, bumpServerFirst, bumpPeek, bumpStare, bumpBump, bumpSplice, bumpTerminate, /*bumpErr,*/ bumpEnd};

/**
 \ingroup  ServerProtocolSSLAPI
 * Short names for ssl-bump modes
 */
extern std::vector<const char *>BumpModeStr;

/**
 \ingroup ServerProtocolSSLAPI
 * Return the short name of the ssl-bump mode "bm"
 */
inline const char *bumpMode(int bm)
{
    return (0 <= bm && bm < Ssl::bumpEnd) ? Ssl::BumpModeStr.at(bm) : nullptr;
}

/// certificates indexed by issuer name
typedef std::multimap<SBuf, X509 *> CertsIndexedList;

/// A successfully extracted/parsed certificate "name" field. See RFC 5280
/// GeneralName and X520CommonName types for examples of information sources.
/// For now, we only support the same two name variants as AnyP::Host:
///
/// * An IPv4 or an IPv6 address. This info comes (with very little validation)
///   from RFC 5280 "iPAddress" variant of a subjectAltName
///
/// * A domain name or domain name wildcard (e.g., *.example.com). This info
///   comes (with very little validation) from a source like these two:
///   - RFC 5280 "dNSName" variant of a subjectAltName extension (GeneralName
///     index is 2, underlying value type is IA5String);
///   - RFC 5280 X520CommonName component of a Subject distinguished name field
///     (underlying value type is DirectoryName).
using GeneralName = AnyP::Host;

/**
 * Load PEM-encoded certificates from the given file.
 */
bool loadCerts(const char *certsFile, Ssl::CertsIndexedList &list);

/**
 * Load PEM-encoded certificates to the squid untrusteds certificates
 * internal DB from the given file.
 */
bool loadSquidUntrusted(const char *path);

/**
 * Removes all certificates from squid untrusteds certificates
 * internal DB and frees all memory
 */
void unloadSquidUntrusted();

/**
 * Add the certificate cert to ssl object untrusted certificates.
 * Squid uses an attached to SSL object list of untrusted certificates,
 * with certificates which can be used to complete incomplete chains sent
 * by the SSL server.
 */
void SSL_add_untrusted_cert(SSL *ssl, X509 *cert);

/// finds certificate issuer URI in the Authority Info Access extension
const char *findIssuerUri(X509 *cert);

/// Searches serverCertificates and local databases for the cert issuer.
/// \param context where to retrieve the configured CA's db; may be nil
/// \returns the found issuer certificate or nil
Security::CertPointer findIssuerCertificate(X509 *cert, const STACK_OF(X509) *serverCertificates, const Security::ContextPointer &context);

/**
 * Fill URIs queue with the uris of missing certificates from serverCertificate chain
 * if this information provided by Authority Info Access.
 \return whether at least one URI is known, including previously known ones
 */
bool missingChainCertificatesUrls(std::queue<SBuf> &URIs, const STACK_OF(X509) &serverCertificates, const Security::ContextPointer &context);

/**
  \ingroup ServerProtocolSSLAPI
  * Generate a certificate to be used as untrusted signing certificate, based on a trusted CA
*/
bool generateUntrustedCert(Security::CertPointer & untrustedCert, Security::PrivateKeyPointer & untrustedPkey, Security::CertPointer const & cert, Security::PrivateKeyPointer const & pkey);

/// certificates indexed by issuer name
typedef std::multimap<SBuf, X509 *> CertsIndexedList;

/**
 \ingroup ServerProtocolSSLAPI
 * Load PEM-encoded certificates from the given file.
 */
bool loadCerts(const char *certsFile, Ssl::CertsIndexedList &list);

/**
 \ingroup ServerProtocolSSLAPI
 * Load PEM-encoded certificates to the squid untrusteds certificates
 * internal DB from the given file.
 */
bool loadSquidUntrusted(const char *path);

/**
 \ingroup ServerProtocolSSLAPI
 * Removes all certificates from squid untrusteds certificates
 * internal DB and frees all memory
 */
void unloadSquidUntrusted();

/**
  \ingroup ServerProtocolSSLAPI
  * Decide on the kind of certificate and generate a CA- or self-signed one
*/
Security::ContextPointer GenerateSslContext(CertificateProperties const &, Security::ServerOptions &, bool trusted);

/**
  \ingroup ServerProtocolSSLAPI
  * Check if the certificate of the given context is still valid
  \param sslContext The context to check
  \param properties Check if the context certificate matches the given properties
  \return true if the contexts certificate is valid, false otherwise
 */
bool verifySslCertificate(const Security::ContextPointer &, CertificateProperties const &);

/**
  \ingroup ServerProtocolSSLAPI
  * Read private key and certificate from memory and generate SSL context
  * using their.
 */
Security::ContextPointer GenerateSslContextUsingPkeyAndCertFromMemory(const char * data, Security::ServerOptions &, bool trusted);

/**
  \ingroup ServerProtocolSSLAPI
  * Create an SSL context using the provided certificate and key
 */
Security::ContextPointer createSSLContext(Security::CertPointer & x509, Security::PrivateKeyPointer & pkey, Security::ServerOptions &);

/**
 \ingroup ServerProtocolSSLAPI
 * Chain signing certificate and chained certificates to an SSL Context
 */
void chainCertificatesToSSLContext(Security::ContextPointer &, Security::ServerOptions &);

/**
 \ingroup ServerProtocolSSLAPI
 * Configure a previously unconfigured SSL context object.
 */
void configureUnconfiguredSslContext(Security::ContextPointer &, Ssl::CertSignAlgorithm signAlgorithm, AnyP::PortCfg &);

/**
  \ingroup ServerProtocolSSLAPI
  * Generates a certificate and a private key using provided properties and set it
  * to SSL object.
 */
bool configureSSL(SSL *ssl, CertificateProperties const &properties, AnyP::PortCfg &port);

/**
  \ingroup ServerProtocolSSLAPI
  * Read private key and certificate from memory and set it to SSL object
  * using their.
 */
bool configureSSLUsingPkeyAndCertFromMemory(SSL *ssl, const char *data, AnyP::PortCfg &port);

/**
  \ingroup ServerProtocolSSLAPI
  * Configures sslContext to use squid untrusted certificates internal list
  * to complete certificate chains when verifies SSL servers certificates.
 */
void useSquidUntrusted(SSL_CTX *sslContext);

/// an algorithm for checking/testing/comparing X.509 certificate names
class GeneralNameMatcher: public Interface
{
public:
    /// whether the given name satisfies algorithm conditions
    bool match(const Ssl::GeneralName &) const;

protected:
    // The methods below implement public match() API for each of the
    // GeneralName variants. For each public match() method call, exactly one of
    // these methods is called.

    virtual bool matchDomainName(const Dns::DomainName &) const = 0;
    virtual bool matchIp(const Ip::Address &) const = 0;
};

/// Determines whether at least one common or alternate subject names matches.
/// The first match (if any) terminates the search.
bool HasMatchingSubjectName(X509 &, const GeneralNameMatcher &);

/// whether at least one common or alternate subject name matches the given one
bool HasSubjectName(X509 &, const AnyP::Host &);

/**
   \ingroup ServerProtocolSSLAPI
   * Convert a given ASN1_TIME to a string form.
   \param tm the time in ASN1_TIME form
   \param buf the buffer to write the output
   \param len write at most len bytes
   \return The number of bytes written
 */
int asn1timeToString(ASN1_TIME *tm, char *buf, int len);

/**
   \ingroup ServerProtocolSSLAPI
   * Sets the hostname for the Server Name Indication (SNI) TLS extension
   * if supported by the used openssl toolkit.
*/
void setClientSNI(SSL *ssl, const char *fqdn);

/**
  \ingroup ServerProtocolSSLAPI
  * Generates a unique key based on CertificateProperties object and store it to key
 */
void InRamCertificateDbKey(const Ssl::CertificateProperties &certProperties, SBuf &key);

/**
  \ingroup ServerProtocolSSLAPI
  Creates and returns an OpenSSL BIO object for writing to `buf` (or throws).
  TODO: Add support for reading from `buf`.
 */
BIO *BIO_new_SBuf(SBuf *buf);

/// Validates the given TLS connection server certificate chain in conjunction
/// with a (possibly empty) set of "extra" intermediate certs. Also consults
/// sslproxy_foreign_intermediate_certs. This is a C++/Squid-friendly wrapper of
/// OpenSSL "verification callback function" (\ref OpenSSL_vcb_disambiguation).
/// OpenSSL has a similar wrapper, ssl_verify_cert_chain(), but that wrapper is
/// not a part of the public OpenSSL API.
bool VerifyConnCertificates(Security::Connection &, const Ssl::X509_STACK_Pointer &extraCerts);

// TODO: Move other ssl_ex_index_* validation-related information here.
/// OpenSSL "verify_callback function" input/output parameters. This information
/// cannot be passed through the verification API directly, so it is aggregated
/// in this class and exchanged via ssl_ex_index_verify_callback_parameters. For
/// OpenSSL validation callback details, see \ref OpenSSL_vcb_disambiguation.
class VerifyCallbackParameters {
public:
    /// creates a VerifyCallbackParameters object and adds it to the given TLS connection
    /// \returns the successfully created and added object
    static VerifyCallbackParameters *New(Security::Connection &);

    /// \returns the VerifyCallbackParameters object previously attached via New()
    static VerifyCallbackParameters &At(Security::Connection &);

    /// \returns the VerifyCallbackParameters object previously attached via New() or nil
    static VerifyCallbackParameters *Find(Security::Connection &);

    /* input parameters */

    /// whether X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY should be cleared
    /// (after setting hidMissingIssuer) because the validation initiator wants
    /// to get the missing certificates and redo the validation with them
    bool callerHandlesMissingCertificates = false;

    /* output parameters */

    /// whether certificate validation has failed due to missing certificate(s)
    /// (i.e. X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY), but the failure was
    /// cleared/hidden due to true callerHandlesMissingCertificates setting; the
    /// certificate chain has to be deemed untrusted until revalidation (if any)
    bool hidMissingIssuer = false;
};

} //namespace Ssl

#if _SQUID_WINDOWS_

#if defined(__cplusplus)

/** \cond AUTODOCS-IGNORE */
namespace Squid
{
/** \endcond */

/// \ingroup ServerProtocolSSLAPI
inline
int SSL_set_fd(SSL *ssl, int fd)
{
    return ::SSL_set_fd(ssl, _get_osfhandle(fd));
}

/// \ingroup ServerProtocolSSLAPI
#define SSL_set_fd(ssl,fd) Squid::SSL_set_fd(ssl,fd)

} /* namespace Squid */

#else

/// \ingroup ServerProtocolSSLAPI
#define SSL_set_fd(s,f) (SSL_set_fd(s, _get_osfhandle(f)))

#endif /* __cplusplus */

#endif /* _SQUID_WINDOWS_ */

#endif /* USE_OPENSSL */
#endif /* SQUID_SRC_SSL_SUPPORT_H */

