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

swish_MetaName *
swish_init_metaname(
    xmlChar *name
)
{
    swish_MetaName *m;
    m = swish_xmalloc(sizeof(swish_MetaName));
    m->ref_cnt = 0;
    m->id = 0;
    m->name = name;
    m->bias = 0;
    m->alias_for = NULL;
    return m;
}

void
swish_debug_metaname(
    swish_MetaName *m
)
{
    SWISH_DEBUG_MSG("\n\
    m->ref_cnt      = %d\n\
    m->id           = %d\n\
    m->name         = %s\n\
    m->bias         = %d\n\
    m->alias_for    = %s\n\
    ", m->ref_cnt, m->id, m->name, m->bias, m->alias_for);
}

void
swish_free_metaname(
    swish_MetaName *m
)
{
    if (m->ref_cnt != 0) {
        SWISH_WARN("MetaName ref_cnt != 0: %d", m->ref_cnt);
    }

    if (m->name != NULL) {
        swish_xfree(m->name);
    }
    if (m->alias_for != NULL) {
        swish_xfree(m->alias_for);
    }

    swish_xfree(m);
}
