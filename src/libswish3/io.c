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
#include <zlib.h>

#include "libswish3.h"

extern int SWISH_DEBUG;
extern int errno;

static void no_nulls(
    xmlChar *filename,
    xmlChar *buffer,
    long bytes_read
);

/* substitute embedded null chars with a newline so we can treat the buffer as a whole
 * string based on similar code in swish-e ver2 file.c */
static void
no_nulls(
    xmlChar *filename,
    xmlChar *buffer,
    long bytes_read
)
{
    if (xmlStrlen(buffer) < bytes_read) {
        long i;
        int j = 0;
        long i_bytes_read = bytes_read;

        for (i = 0; i < i_bytes_read; ++i) {
            if (buffer[i] == '\0') {
                buffer[i] = '\n';
                j++;
            }
            if (buffer[i] == SWISH_TOKENPOS_BUMPER[0]) {
                buffer[i] = '\n';
                j++;
            }
        }

        if (j) {
            SWISH_WARN
                ("Substituted %d embedded null or connector character(s) in file '%s' with newline(s)",
                 j, filename);
        }
    }

}

xmlChar *
swish_io_slurp_fh(
    FILE * fh,
    unsigned long flen,
    boolean binmode
)
{
    size_t bytes_read;
    xmlChar *buffer;

/* printf("slurping %d bytes\n", flen); */

    buffer = swish_xmalloc(flen + 1);
    *buffer = '\0';

    bytes_read = fread(buffer, sizeof(xmlChar), flen, fh);

    if (bytes_read != flen) {
        SWISH_CROAK("did not read expected bytes: %ld expected, %d read", flen,
                    bytes_read);
    }
    buffer[bytes_read] = '\0';  /* terminate the string */

/* printf("read %d bytes from stdin\n", bytes_read); */

    if (!binmode) {
        no_nulls((xmlChar *)"filehandle", buffer, (int)bytes_read);
    }
    
    return buffer;
}

xmlChar *
swish_io_slurp_file_len(
    xmlChar *filename,
    off_t flen,
    boolean binmode
)
{
    size_t bytes_read;
    FILE *fp;
    xmlChar *buffer;

    if (flen > SWISH_MAX_FILE_LEN) {
        flen = SWISH_MAX_FILE_LEN;
        SWISH_WARN("max file len %ld exceeded - cannot read %ld bytes from %s",
                   SWISH_MAX_FILE_LEN, flen, filename);

    }
    
    if (SWISH_DEBUG & SWISH_DEBUG_IO)
        SWISH_DEBUG_MSG("slurp file '%s' [%ld bytes]", filename, flen);

    buffer = swish_xmalloc(flen + 1);

    if ((fp = fopen((char *)filename, "r")) == 0) {
        SWISH_CROAK("Error reading file %s: %s", filename, strerror(errno));
    }

    bytes_read = fread(buffer, sizeof(xmlChar), flen, fp);

    if (bytes_read != flen) {
        SWISH_CROAK("did not read expected bytes: %ld expected, %d read (%s)",
                    flen, bytes_read, strerror(errno));
    }
    buffer[bytes_read] = '\0';  /* terminate the string */

/* close the stream */
    if (fclose(fp))
        SWISH_CROAK("error closing filehandle for %s: %s", 
            filename, strerror(errno));

    if (!binmode) {
        no_nulls(filename, buffer, (long)bytes_read);
    }
    
    return buffer;
}

xmlChar *
swish_io_slurp_gzfile_len(
    xmlChar *filename,
    off_t *flen,
    boolean binmode
)
{
    off_t bytes_read, buffer_len;
    int ret;
    gzFile fh;
    xmlChar *buffer;
    unsigned int buf_size;
    int compression_rate = 3;   /* seems about right */
    
    buf_size = sizeof(xmlChar)*(*flen)*compression_rate;
    buffer = swish_xmalloc(buf_size);
    buffer_len = 0;
    fh = gzopen((char*)filename, "r");
    if (fh == NULL) {
        SWISH_CROAK("Failed to open file '%s' for read: %s",
            filename, strerror(errno));
    }
    while ((bytes_read = gzread(fh, buffer, buf_size)) != 0) {
        if (bytes_read == -1) {
            SWISH_CROAK("Error reading gzipped file '%s': %s",
                filename, strerror(errno));
        }
        if (SWISH_DEBUG & SWISH_DEBUG_IO) {
            SWISH_DEBUG_MSG("Read %d bytes from %s", bytes_read, filename);
        }
        if (bytes_read < buf_size) {
            if (SWISH_DEBUG & SWISH_DEBUG_IO) {
                SWISH_DEBUG_MSG("Read to end of file");
            }
            buffer_len = bytes_read;
            break;
        }
        buf_size *= compression_rate;
        buffer = swish_xrealloc(buffer, buf_size);
        if (SWISH_DEBUG & SWISH_DEBUG_IO) {
            SWISH_DEBUG_MSG("grew buffer to %d", buf_size);
        }
        buffer_len = bytes_read;
        ret = gzrewind(fh);
        if (SWISH_DEBUG & SWISH_DEBUG_IO) {
            SWISH_DEBUG_MSG("gzrewind ret = %d", ret);
        }
    }
    ret = gzclose(fh);    // TODO check for err?
        
    buffer[buffer_len] = '\0';
    
    if (!binmode) {
        no_nulls(filename, buffer, (long)buffer_len);
    }
   
    if (SWISH_DEBUG & SWISH_DEBUG_IO) { 
        SWISH_DEBUG_MSG("slurped gzipped file '%s' buffer_len=%d buf_size=%d orig flen=%d", 
            filename, buffer_len, buf_size, *flen);
    }

    /* set the flen pointer to the actual length */
    *flen = buffer_len;
      
    return buffer;
}

xmlChar *
swish_io_slurp_file(
    xmlChar *filename,
    off_t file_len,
    boolean is_gzipped,
    boolean binmode
)
{
    if (!file_len) {
        file_len = swish_fs_get_file_size(filename);
    }
    if (!file_len || file_len == -1) {
        SWISH_CROAK("Can't stat %s: %s\n", filename, strerror(errno));
    }
    if (is_gzipped) {
        return swish_io_slurp_gzfile_len(filename, &file_len, binmode);
    }
    else {
        return swish_io_slurp_file_len(filename, file_len, binmode);
    }
}

long int
swish_io_count_operable_file_lines(
    xmlChar *filename
)
{
    long int count;
    FILE *fp;
    xmlChar line_in_file[SWISH_MAXSTRLEN];
    
    count = 0;
    
    fp = fopen((const char*)filename, "r");
    if (fp == NULL) {
        SWISH_CROAK("failed to open file: %s", filename);
    }
    while (fgets((char*)line_in_file, SWISH_MAXSTRLEN, fp) != NULL) {
        if (swish_io_is_skippable_line(line_in_file))  
            continue;
        
        count++;
        //SWISH_DEBUG_MSG("count %d for '%s'", count, line_in_file);

    }
    
    if (fclose(fp)) {
        SWISH_CROAK("error closing filelist");
    }

    return count;
}


boolean
swish_io_is_skippable_line(
    xmlChar *str
)
{
    xmlChar *line;

    /* skip leading white space */
    line = swish_str_skip_ws(str);
    
    //SWISH_DEBUG_MSG("line: '%s'", line);
        
    if (xmlStrlen(line) == 0 || (xmlStrlen(line) == 1 && line[0] == '\n')) {
        /* blank line */
        return SWISH_TRUE;
    }
    if (line[0] == '#') {
        /* skip comments */
        return SWISH_TRUE;
    }
    
    return SWISH_FALSE;
}
