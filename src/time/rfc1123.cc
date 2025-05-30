/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "time/gadgets.h"

/*
 *  Adapted from HTSUtils.c in CERN httpd 3.0 (http://info.cern.ch/httpd/)
 *  by Darren Hardy <hardy@cs.colorado.edu>, November 1994.
 */
#include <cctype>
#include <cstring>
#include <ctime>

#define RFC850_STRFTIME "%A, %d-%b-%y %H:%M:%S GMT"
#define RFC1123_STRFTIME "%a, %d %b %Y %H:%M:%S GMT"

static int make_month(const char *s);
static int make_num(const char *s);

static const char *month_names[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int
make_num(const char *s)
{
    if (*s >= '0' && *s <= '9')
        return 10 * (*s - '0') + *(s + 1) - '0';
    else
        return *(s + 1) - '0';
}

static int
make_month(const char *s)
{
    int i;
    char month[3];

    month[0] = xtoupper(*s);
    if (!month[0])
        return -1; // protects *(s + 1) below

    month[1] = xtolower(*(s + 1));
    if (!month[1])
        return -1; // protects *(s + 2) below

    month[2] = xtolower(*(s + 2));

    for (i = 0; i < 12; i++)
        if (!strncmp(month_names[i], month, 3))
            return i;
    return -1;
}

static int
tmSaneValues(struct tm *tm)
{
    if (tm->tm_sec < 0 || tm->tm_sec > 59)
        return 0;
    if (tm->tm_min < 0 || tm->tm_min > 59)
        return 0;
    if (tm->tm_hour < 0 || tm->tm_hour > 23)
        return 0;
    if (tm->tm_mday < 1 || tm->tm_mday > 31)
        return 0;
    if (tm->tm_mon < 0 || tm->tm_mon > 11)
        return 0;
    return 1;
}

static struct tm *
parse_date_elements(const char *day, const char *month, const char *year,
                    const char *aTime, const char *zone) {
    static struct tm tm;
    const char *t;
    memset(&tm, 0, sizeof(tm));

    if (!day || !month || !year || !aTime || (zone && strcmp(zone, "GMT")))
        return nullptr;
    tm.tm_mday = atoi(day);
    tm.tm_mon = make_month(month);
    if (tm.tm_mon < 0)
        return nullptr;
    tm.tm_year = atoi(year);
    if (strlen(year) == 4)
        tm.tm_year -= 1900;
    else if (tm.tm_year < 70)
        tm.tm_year += 100;
    else if (tm.tm_year > 19000)
        tm.tm_year -= 19000;
    tm.tm_hour = make_num(aTime);
    t = strchr(aTime, ':');
    if (!t)
        return nullptr;
    t++;
    tm.tm_min = atoi(t);
    t = strchr(t, ':');
    if (t)
        tm.tm_sec = atoi(t + 1);
    return tmSaneValues(&tm) ? &tm : nullptr;
}

static struct tm *
parse_date(const char *str) {
    struct tm *tm;
    static char tmp[64];
    char *t;
    char *wday = nullptr;
    char *day = nullptr;
    char *month = nullptr;
    char *year = nullptr;
    char *timestr = nullptr;
    char *zone = nullptr;

    xstrncpy(tmp, str, 64);

    for (t = strtok(tmp, ", "); t; t = strtok(nullptr, ", ")) {
        if (xisdigit(*t)) {
            if (!day) {
                day = t;
                t = strchr(t, '-');
                if (t) {
                    *t++ = '\0';
                    month = t;
                    t = strchr(t, '-');
                    if (!t)
                        return nullptr;
                    *t++ = '\0';
                    year = t;
                }
            } else if (strchr(t, ':'))
                timestr = t;
            else if (!year)
                year = t;
            else
                return nullptr;
        } else if (!wday)
            wday = t;
        else if (!month)
            month = t;
        else if (!zone)
            zone = t;
        else
            return nullptr;
    }
    tm = parse_date_elements(day, month, year, timestr, zone);

    return tm;
}

time_t
Time::ParseRfc1123(const char *str)
{
    struct tm *tm;
    time_t t;
    if (nullptr == str)
        return -1;
    tm = parse_date(str);
    if (!tm)
        return -1;
    tm->tm_isdst = -1;
#if HAVE_TIMEGM
    t = timegm(tm);
#elif HAVE_TM_TM_GMTOFF
    t = mktime(tm);
    if (t != -1) {
        struct tm *local = localtime(&t);
        t += local->tm_gmtoff;
    }
#else
    /* some systems do not have tm_gmtoff so we fake it */
    t = mktime(tm);
    if (t != -1) {
        time_t dst = 0;
#if !(defined(_TIMEZONE) || defined(_timezone) || _SQUID_AIX_ || _SQUID_WINDOWS_ || _SQUID_SGI_)
        extern long timezone;
#endif
        /*
         * The following assumes a fixed DST offset of 1 hour,
         * which is probably wrong.
         */
        if (tm->tm_isdst > 0)
            dst = -3600;
#if defined(_timezone) || _SQUID_WINDOWS_
        t -= (_timezone + dst);
#else
        t -= (timezone + dst);
#endif
    }
#endif
    return t;
}

const char *
Time::FormatRfc1123(time_t t)
{
    static char buf[128];

    struct tm *gmt = gmtime(&t);

    buf[0] = '\0';
    strftime(buf, 127, RFC1123_STRFTIME, gmt);
    return buf;
}

