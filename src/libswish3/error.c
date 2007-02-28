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

/* error handling based on Swish-e ver2 error.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <libxml/globals.h>

#include "libswish3.h"

extern int      SWISH_DEBUG;


static FILE *error_handle = NULL;

void swish_set_error_handle( FILE *where )
{
    error_handle = where;
}

void swish_fatal_err(char *msgfmt,...)
{
  va_list args;

  if ( !error_handle )
      error_handle = stderr;

  va_start (args,msgfmt);
  fprintf  (error_handle, "Swish ERROR: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "\n");
  va_end   (args);

  if(!errno)
    errno = 1;
    
  exit(errno);
}

void swish_warn_err(char *msgfmt,...)
{
  va_list args;

  if ( !error_handle )
      error_handle = stderr;

  va_start (args,msgfmt);
  fprintf  (error_handle, "Swish WARNING: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "\n");
  va_end   (args);
}

void swish_debug_msg(char *msgfmt,...)
{
  va_list args;

  if ( !error_handle )
      error_handle = stderr;

  va_start (args,msgfmt);
  fprintf  (error_handle, "Swish DEBUG: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "\n");
  va_end   (args);
}
