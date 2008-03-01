/*
 * This file is part of libswish3
 * Copyright (C) 2008 Peter Karman
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

#include "libswish3.h"

extern int SWISH_DEBUG;

swish_Property *
swish_init_property( xmlChar *name )
{
    swish_Property *prop;
    prop = swish_xmalloc(sizeof(swish_Property));
    prop->ref_cnt       = 0;
    prop->id            = 0;
    prop->name          = name;
    prop->ignore_case   = 1;
    prop->type          = SWISH_PROP_STRING;
    prop->verbatim      = 0;
    prop->alias_for     = NULL;
    prop->max           = 0;
    prop->sort          = 0;
    return prop;
}

void
swish_debug_property( swish_Property * p )
{
    SWISH_DEBUG_MSG("\n\
    prop->ref_cnt       = %d\n\
    prop->id            = %d\n\
    prop->name          = %s\n\
    prop->ignore_case   = %d\n\
    prop->type          = %d\n\
    prop->verbatim      = %d\n\
    prop->alias_for     = %s\n\
    prop->max           = %d\n\
    prop->sort          = %d\n\
    ", p->ref_cnt, p->id, p->name, p->ignore_case,
    p->type, p->verbatim, p->alias_for, p->max, p->sort);
}

void
swish_free_property( swish_Property * p )
{
    if (p->ref_cnt != 0) {
        SWISH_WARN("Property ref_cnt != 0: %d", p->ref_cnt);
    }
    
    if (p->name != NULL) {
        swish_xfree(p->name);
    }
    if (p->alias_for != NULL) {
        swish_xfree(p->alias_for);
    }
    
    swish_xfree(p);
}
