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

/* simple I/O functions */

#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <libxml/globals.h>

#include "libswish3.h"

extern int      SWISH_DEBUG;
extern int      errno;


/* substitute embedded null chars with a newline so we can treat the buffer as a whole
 * string based on similar code in swish-e ver2 file.c */
static void 
no_nulls(
     xmlChar * filename,
     xmlChar * buffer,
     int bytes_read
)
{
    if (xmlStrlen(buffer) < bytes_read)
    {
        int             i;
        int             j = 0;
        int             i_bytes_read = (int) bytes_read;

        for (i = 0; i < i_bytes_read; ++i)
        {
            if (buffer[i] == '\0')
            {
                buffer[i] = '\n';
                j++;
            }
            if (    buffer[i] == SWISH_META_CONNECTOR[0]
                ||  buffer[i] == SWISH_META_CONNECTOR[0]
                )
            {
                buffer[i] = '\n';
                j++;
            }
        }

        if (j)
            swish_warn_err(
                    "Substituted %d embedded null or connector character(s) in file '%s' with newline(s)\n",
                     j, filename);
    }

}


xmlChar        *
swish_slurp_fh(FILE * fh, long flen)
{

    size_t          bytes_read;
    xmlChar        *buffer;

    /* printf("slurping %d bytes\n", flen); */

    buffer = swish_xmalloc(flen + 1);
    *buffer = '\0';

    bytes_read = fread(buffer, sizeof(xmlChar), flen, fh);

    if (bytes_read != flen)
    {
        swish_fatal_err("did not read expected bytes: %ld expected, %d read\n", flen, bytes_read);
    }
    buffer[bytes_read] = '\0';    /* terminate the string */

    /* printf("read %d bytes from stdin\n", bytes_read); */

    no_nulls((xmlChar*)"filehandle", buffer, (int)bytes_read);

    return buffer;
}



xmlChar        *
swish_slurp_file_len(xmlChar * filename, long flen)
{
    size_t          bytes_read;
    FILE           *fp;
    xmlChar        *buffer;

    if (flen > SWISH_MAX_FILE_LEN)
    {
        flen = SWISH_MAX_FILE_LEN;
        swish_warn_err("max file len %ld exceeded - cannot read %ld bytes from %s",
             SWISH_MAX_FILE_LEN, flen, filename);

    }

    buffer = swish_xmalloc(flen + 1);

    if ((fp = fopen((char *) filename, "r")) == 0)
    {
        swish_fatal_err("Error reading file %s: %s\n", 
                            filename, strerror(errno));
    }

    bytes_read = fread(buffer, sizeof(xmlChar), flen, fp);

    if (bytes_read != flen)
    {
        swish_fatal_err("did not read expected bytes: %ld expected, %d read (%s)\n", 
                            flen, bytes_read, strerror(errno));
    }
    buffer[bytes_read] = '\0';    /* terminate the string */

    /* close the stream */
    if (fclose(fp))
        swish_fatal_err("error closing filehandle for %s: %s\n", 
                            filename, strerror(errno));

    no_nulls(filename, buffer, (int)bytes_read);

    return buffer;

}


xmlChar        *
swish_slurp_file(xmlChar * filename)
{
    struct stat     info;
    /* fatal error, since we can't proceed */
    if (stat((char *) filename, &info))
    {
        swish_fatal_err("Can't stat %s: %s\n", filename, strerror(errno));
    }
    return swish_slurp_file_len(filename, info.st_size);

}

