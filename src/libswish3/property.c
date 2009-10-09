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
swish_property_init(
    xmlChar *name
)
{
    swish_Property *p;
    p = swish_xmalloc(sizeof(swish_Property));
    p->ref_cnt = 0;
    p->id = -1;
    p->name = name;
    p->ignore_case = SWISH_TRUE;
    p->type = SWISH_PROP_STRING;
    p->verbatim = SWISH_FALSE;
    p->alias_for = NULL;
    p->max = 0;
    p->sort = SWISH_FALSE;
    return p;
}

void
swish_property_debug(
    swish_Property *p
)
{
    SWISH_DEBUG_MSG("\n\
    p->ref_cnt       = %d\n\
    p->id            = %d\n\
    p->name          = %s\n\
    p->ignore_case   = %d\n\
    p->type          = %d\n\
    p->verbatim      = %d\n\
    p->alias_for     = %s\n\
    p->max           = %d\n\
    p->sort          = %d\n\
    ", p->ref_cnt, p->id, p->name, p->ignore_case, p->type, p->verbatim, p->alias_for, p->max, p->sort);
}

void
swish_property_free(
    swish_Property *p
)
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

int
swish_property_get_id(
    xmlChar *propname, 
    xmlHashTablePtr properties
)
{
    int prop_id;
    swish_Property *prop;
    
    // special cases
    if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_RANK)) {
        prop_id = SWISH_PROP_RANK_ID;
    }
    else if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_DOCPATH)) {
        prop_id = SWISH_PROP_DOCPATH_ID;
    }
    else if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_MTIME)) {
        prop_id = SWISH_PROP_MTIME_ID;
    }
    else if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_SIZE)) {
        prop_id = SWISH_PROP_SIZE_ID;
    }
    else if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_MIME)) {
        prop_id = SWISH_PROP_MIME_ID;
    }
    else if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_PARSER)) {
        prop_id = SWISH_PROP_PARSER_ID;
    }
    else if (xmlStrEqual(propname, BAD_CAST SWISH_PROP_NWORDS)) {
        prop_id = SWISH_PROP_NWORDS_ID;
    }
    
    // look up the propname in the config
    else if (swish_hash_exists( properties, propname )) {
        prop = (swish_Property*)swish_hash_fetch( properties, propname );
        prop_id = prop->id;
    }
    else {
        SWISH_CROAK("No such PropertyName: %s", propname);
    }

    return prop_id;
}
