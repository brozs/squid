/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "HttpReply.h"

#define STUB_API "HttpReply.cc"
#include "tests/STUB.h"

HttpReply::HttpReply() : Http::Message(hoReply), date (0), last_modified (0),
    expires(0), surrogate_control(nullptr), keep_alive(0),
    protoPrefix("HTTP/"), do_clean(false), bodySizeMax(-2), content_range(nullptr)
{STUB_NOP}
HttpReply::~HttpReply() STUB
void HttpReply::setHeaders(Http::StatusCode, const char *, const char *, int64_t, time_t, time_t) STUB
void HttpReply::packHeadersUsingFastPacker(Packable&) const STUB
void HttpReply::packHeadersUsingSlowPacker(Packable&) const STUB
void HttpReply::reset() STUB
bool HttpReply::sanityCheckStartLine(const char *, const size_t, Http::StatusCode *) STUB_RETVAL(false)
int HttpReply::httpMsgParseError() STUB_RETVAL(0)
bool HttpReply::expectingBody(const HttpRequestMethod&, int64_t&) const STUB_RETVAL(false)
size_t HttpReply::parseTerminatedPrefix(const char *, size_t) STUB_RETVAL(0)
size_t HttpReply::prefixLen() const STUB_RETVAL(0)
bool HttpReply::parseFirstLine(const char *, const char *) STUB_RETVAL(false)
void HttpReply::hdrCacheInit() STUB
HttpReply * HttpReply::clone() const STUB_RETVAL(nullptr)
bool HttpReply::inheritProperties(const Http::Message *) STUB_RETVAL(false)
HttpReply::Pointer HttpReply::recreateOnNotModified(const HttpReply &) const STUB_RETVAL(nullptr)
int64_t HttpReply::bodySize(const HttpRequestMethod&) const STUB_RETVAL(0)
const HttpHdrContRange *HttpReply::contentRange() const STUB_RETVAL(nullptr)
void HttpReply::configureContentLengthInterpreter(Http::ContentLengthInterpreter &) STUB

