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

/* MIME types hash based on Apache 2.0 mime.types listing
see <http://www.iana.org/assignments/media-types/> for official registry.
*/

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

//should be total number of strings(NOT pairs !) below
#define SWISH_MIME_TABLE_COUNT  304

static char *SWISH_MIME_TABLE[] = {
    "ai", "application/postscript",
    "aif", "audio/x-aiff",
    "aifc", "audio/x-aiff",
    "aiff", "audio/x-aiff",
    "asc", "text/plain",
    "au", "audio/basic",
    "avi", "video/x-msvideo",
    "bcpio", "application/x-bcpio",
    "bin", "application/octet-stream",
    "bmp", "image/bmp",
    "cdf", "application/x-netcdf",
    "cgm", "image/cgm",
    "class", "application/octet-stream",
    "cpio", "application/x-cpio",
    "cpt", "application/mac-compactpro",
    "csh", "application/x-csh",
    "css", "text/css",
    "dcr", "application/x-director",
    "dir", "application/x-director",
    "djv", "image/vnd.djvu",
    "djvu", "image/vnd.djvu",
    "dll", "application/octet-stream",
    "dmg", "application/octet-stream",
    "dms", "application/octet-stream",
    "doc", "application/msword",
    "dtd", "application/xml-dtd",
    "dvi", "application/x-dvi",
    "dxr", "application/x-director",
    "eps", "application/postscript",
    "etx", "text/x-setext",
    "exe", "application/octet-stream",
    "ez", "application/andrew-inset",
    "gif", "image/gif",
    "gram", "application/srgs",
    "grxml", "application/srgs+xml",
    "gtar", "application/x-gtar",
    "gz", "application/x-gzip",
    "hdf", "application/x-hdf",
    "hqx", "application/mac-binhex40",
    "htm", "text/html",
    "html", "text/html",
    "ice", "x-conference/x-cooltalk",
    "ico", "image/x-icon",
    "ics", "text/calendar",
    "ief", "image/ief",
    "ifb", "text/calendar",
    "iges", "model/iges",
    "igs", "model/iges",
    "jpe", "image/jpeg",
    "jpeg", "image/jpeg",
    "jpg", "image/jpeg",
    "js", "application/x-javascript",
    "kar", "audio/midi",
    "latex", "application/x-latex",
    "lha", "application/octet-stream",
    "lzh", "application/octet-stream",
    "m3u", "audio/x-mpegurl",
    "m4u", "video/vnd.mpegurl",
    "man", "application/x-troff-man",
    "mathml", "application/mathml+xml",
    "me", "application/x-troff-me",
    "mesh", "model/mesh",
    "mid", "audio/midi",
    "midi", "audio/midi",
    "mif", "application/vnd.mif",
    "mov", "video/quicktime",
    "movie", "video/x-sgi-movie",
    "mp2", "audio/mpeg",
    "mp3", "audio/mpeg",
    "mpe", "video/mpeg",
    "mpeg", "video/mpeg",
    "mpg", "video/mpeg",
    "mpga", "audio/mpeg",
    "ms", "application/x-troff-ms",
    "msh", "model/mesh",
    "mxu", "video/vnd.mpegurl",
    "nc", "application/x-netcdf",
    "oda", "application/oda",
    "ogg", "application/ogg",
    "pbm", "image/x-portable-bitmap",
    "pdb", "chemical/x-pdb",
    "pdf", "application/pdf",
    "pgm", "image/x-portable-graymap",
    "pgn", "application/x-chess-pgn",
    "png", "image/png",
    "pnm", "image/x-portable-anymap",
    "ppm", "image/x-portable-pixmap",
    "ppt", "application/vnd.ms-powerpoint",
    "ps", "application/postscript",
    "qt", "video/quicktime",
    "ra", "audio/x-pn-realaudio",
    "ram", "audio/x-pn-realaudio",
    "ras", "image/x-cmu-raster",
    "rdf", "application/rdf+xml",
    "rgb", "image/x-rgb",
    "rm", "application/vnd.rn-realmedia",
    "roff", "application/x-troff",
    "rtf", "text/rtf",
    "rtx", "text/richtext",
    "sgm", "text/sgml",
    "sgml", "text/sgml",
    "sh", "application/x-sh",
    "shar", "application/x-shar",
    "silo", "model/mesh",
    "sit", "application/x-stuffit",
    "skd", "application/x-koan",
    "skm", "application/x-koan",
    "skp", "application/x-koan",
    "skt", "application/x-koan",
    "smi", "application/smil",
    "smil", "application/smil",
    "snd", "audio/basic",
    "so", "application/octet-stream",
    "spl", "application/x-futuresplash",
    "src", "application/x-wais-source",
    "sv4cpio", "application/x-sv4cpio",
    "sv4crc", "application/x-sv4crc",
    "svg", "image/svg+xml",
    "swf", "application/x-shockwave-flash",
    "t", "application/x-troff",
    "tar", "application/x-tar",
    "tcl", "application/x-tcl",
    "tex", "application/x-tex",
    "texi", "application/x-texinfo",
    "texinfo", "application/x-texinfo",
    "tif", "image/tiff",
    "tiff", "image/tiff",
    "tr", "application/x-troff",
    "tsv", "text/tab-separated-values",
    "txt", "text/plain",
    "ustar", "application/x-ustar",
    "vcd", "application/x-cdlink",
    "vrml", "model/vrml",
    "vxml", "application/voicexml+xml",
    "wav", "audio/x-wav",
    "wbmp", "image/vnd.wap.wbmp",
    "wbxml", "application/vnd.wap.wbxml",
    "wml", "text/vnd.wap.wml",
    "wmlc", "application/vnd.wap.wmlc",
    "wmls", "text/vnd.wap.wmlscript",
    "wmlsc", "application/vnd.wap.wmlscriptc",
    "wrl", "model/vrml",
    "xbm", "image/x-xbitmap",
    "xht", "application/xhtml+xml",
    "xhtml", "application/xhtml+xml",
    "xls", "application/vnd.ms-excel",
/* "xml",       "application/xml", */
    "xml", "text/xml",
    "xpm", "image/x-xpixmap",
    "xsl", "application/xml",
    "xslt", "application/xslt+xml",
    "xul", "application/vnd.mozilla.xul+xml",
    "xwd", "image/x-xwindowdump",
    "xyz", "chemical/x-xyz",
    "zip", "application/zip"
};

/* create hash of file ext => mime type */
xmlHashTablePtr
swish_mime_hash(
)
{
    int i;
    xmlHashTablePtr mimes;
    mimes = xmlHashCreate(SWISH_MIME_TABLE_COUNT / 2);

    for (i = 0; i <= SWISH_MIME_TABLE_COUNT; i += 2) {
        swish_hash_add(mimes, (xmlChar *)SWISH_MIME_TABLE[i],
                       swish_xstrdup((xmlChar *)SWISH_MIME_TABLE[i + 1])
            );
    }

    return mimes;
}

/* retrieve mime type from hash */
xmlChar *
swish_get_mime_type(
    swish_Config *config,
    xmlChar *fileext
)
{
    xmlChar *mime;
    mime = swish_hash_fetch(config->mimes, fileext);
    if (mime == NULL) {
        SWISH_WARN("No MIME type known for '%s' -- using '%s'", fileext,
                   SWISH_DEFAULT_MIME);
        mime = swish_xstrdup((xmlChar *)SWISH_DEFAULT_MIME);
    }
    return swish_xstrdup(mime);
}

/* returns parser type (TXT, HTML, XML) based on mime type */
xmlChar *
swish_get_parser(
    swish_Config *config,
    xmlChar *mime
)
{
    xmlChar *parser;
    xmlChar *deftype;

    parser = swish_hash_fetch(config->parsers, mime);

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO)
        SWISH_DEBUG_MSG("using parser '%s' based on MIME '%s'", parser, mime);

    deftype = swish_hash_fetch(config->parsers, (xmlChar *)SWISH_DEFAULT_PARSER);       /* error check?? */

    if (parser == NULL) {
        SWISH_WARN("No parser for MIME '%s' -- using '%s'", mime, deftype);
        parser = deftype;
    }

    return swish_xstrdup(parser);       /* so we don't change orig value -- MUST free */
}
