/*
 * This file is part of libswish3
 * Copyright (C) 2007 Peter Karman
 *
 *  libswish3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libswish3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libswish3; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/* based on Swish-e version 2 */

#include <stdio.h>
#include <time.h>
#include "getruntime.c"
#include "libswish3.h"

/*
  -- TimeHiRes returns a ClockTick value (double)
  -- in seconds.fractions
*/

#ifdef HAVE_BSDGETTIMEOFDAY
#define gettimeofday BSDgettimeofday
#endif

#ifdef NO_GETTOD

double
swish_time_elapsed(void)
{
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>

    struct timeb    ftimebuf;

    ftime(&ftimebuf);
    return (double) ftimebuf.time + (double) ftimebuf.millitm / 1000.0;

#else

                    return ((double) clock()) / CLOCKS_PER_SEC;

#endif
}

#else

#include <sys/time.h>

double
swish_time_elapsed(void)
{
    struct timeval  t;
    int             i;

    i = gettimeofday(&t, NULL);
    if (i)
        return 0;

    return (double) (t.tv_sec + t.tv_usec / 1000000.0);
}
#endif

/* return CPU time used */
double
swish_time_cpu(void)
{
    return (double) get_cpu_secs();
}

char           *
swish_print_time(double time)
{
    int             hh, mm, ss;
    int             delta;
    char           *str;

    if (time < 0)
        time = 0;

    delta = (int) (time + 0.5);
    ss = delta % 60;
    delta /= 60;
    hh = delta / 60;
    mm = delta % 60;

    str = swish_xmalloc(9);
    if (sprintf(str, "%02d:%02d:%02d", hh, mm, ss) > 0) {
        return str;
    }
    else {
        swish_xfree(str);
        return (char *) swish_xstrdup((xmlChar *) "unknown time");
    }
}

char           *
swish_print_fine_time(double time)
{
    char           *str;

    if (time >= 10)
        time = 9.99999;

    str = swish_xmalloc(8);
    if (sprintf(str, "%1.5f", time) > 0)
        return str;

    else {
        swish_xfree(str);
        return (char *) swish_xstrdup((xmlChar *) "unknown fine time");
    }

}
