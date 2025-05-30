/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 50    Log file handling */

#include "squid.h"
#include "debug/Messages.h"
#include "fatal.h"
#include "fde.h"
#include "log/File.h"
#include "log/ModDaemon.h"
#include "log/ModStdio.h"
#include "log/ModSyslog.h"
#include "log/ModUdp.h"
#include "log/TcpLogger.h"
#include "sbuf/SBuf.h"

CBDATA_CLASS_INIT(Logfile);

Logfile::Logfile(const char *aPath) :
    sequence_number(0),
    data(nullptr),
    f_linestart(nullptr),
    f_linewrite(nullptr),
    f_lineend(nullptr),
    f_flush(nullptr),
    f_rotate(nullptr),
    f_close(nullptr)
{
    xstrncpy(path, aPath, sizeof(path));
    flags.fatal = 0;
}

Logfile *
logfileOpen(const char *path, size_t bufsz, int fatal_flag)
{
    int ret;
    const char *patharg;

    debugs(50, Important(26), "Logfile: opening log " << path);

    Logfile *lf = new Logfile(path);
    patharg = path;
    /* need to call the per-logfile-type code */
    if (strncmp(path, "stdio:", 6) == 0) {
        patharg = path + 6;
        ret = logfile_mod_stdio_open(lf, patharg, bufsz, fatal_flag);
    } else if (strncmp(path, "daemon:", 7) == 0) {
        patharg = path + 7;
        ret = logfile_mod_daemon_open(lf, patharg, bufsz, fatal_flag);
    } else if (strncmp(path, "tcp:", 4) == 0) {
        patharg = path + 4;
        ret = Log::TcpLogger::Open(lf, patharg, bufsz, fatal_flag);
    } else if (strncmp(path, "udp:", 4) == 0) {
        patharg = path + 4;
        ret = logfile_mod_udp_open(lf, patharg, bufsz, fatal_flag);
#if HAVE_SYSLOG
    } else if (strncmp(path, "syslog:", 7) == 0) {
        patharg = path + 7;
        ret = logfile_mod_syslog_open(lf, patharg, bufsz, fatal_flag);
#endif
    } else {
        debugs(50, DBG_IMPORTANT, "WARNING: log name now starts with a module name. Use 'stdio:" << patharg << "'");
        snprintf(lf->path, MAXPATHLEN, "stdio:%s", patharg);
        ret = logfile_mod_stdio_open(lf, patharg, bufsz, fatal_flag);
    }
    if (!ret) {
        if (fatal_flag)
            fatalf("logfileOpen: %s: couldn't open!\n", path);
        else
            debugs(50, DBG_IMPORTANT, "ERROR: logfileOpen: " << path << ": could not open!");
        lf->f_close(lf);
        delete lf;
        return nullptr;
    }
    assert(lf->data != nullptr);

    if (fatal_flag)
        lf->flags.fatal = 1;

    lf->sequence_number = 0;

    return lf;
}

void
logfileClose(Logfile * lf)
{
    debugs(50, Important(27), "Logfile: closing log " << lf->path);
    lf->f_flush(lf);
    lf->f_close(lf);
    delete lf;
}

void
logfileRotate(Logfile * lf, int16_t rotateCount)
{
    debugs(50, DBG_IMPORTANT, "logfileRotate: " << lf->path);
    lf->f_rotate(lf, rotateCount);
}

void
logfileWrite(Logfile * lf, const char *buf, size_t len)
{
    lf->f_linewrite(lf, buf, len);
}

void
logfilePrintf(Logfile * lf, const char *fmt,...)
{
    va_list args;
    va_start(args, fmt);
    static SBuf sbuf;
    sbuf.clear();
    sbuf.vappendf(fmt, args); // Throws on overflow. TODO: handle that better
    logfileWrite(lf, sbuf.c_str(), sbuf.length());
    va_end(args);
}

void
logfileLineStart(Logfile * lf)
{
    lf->f_linestart(lf);
}

void
logfileLineEnd(Logfile * lf)
{
    lf->f_lineend(lf);
    ++ lf->sequence_number;
}

void
logfileFlush(Logfile * lf)
{
    lf->f_flush(lf);
}

