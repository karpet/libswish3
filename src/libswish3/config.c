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

/* parse XML-style config files
 * 
 * based on http://www.yolinux.com/TUTORIALS/GnomeLibXml2.html
 * 
 */


/* #include <libxml/hash.h> */
#include <libxml/xmlstring.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"

extern int      SWISH_DEBUG;


static void     config_val_printer(xmlChar * val, xmlChar * str, xmlChar * key);
static void     config_printer(xmlHashTablePtr vhash, xmlChar * str, xmlChar * key);
static void     free_config1(void *payload, xmlChar * confName);
static void     free_config2(void *payload, xmlChar * key);
static xmlDocPtr parse_xml_config(xmlChar * conf);
static int      is_multi(xmlNode * node);
static int      is_equal(xmlNode * node);
static int      is_top_level(xmlNode * node);
static xmlChar *get_node_value(xmlNode * node);
static xmlChar *get_node_key(xmlNode * node);
static xmlChar *get_node_type(xmlNode * node);
static xmlNode *get_dom_root(xmlDocPtr doc);
static int      node_has_key(xmlNode * node);
static int      node_has_value(xmlNode * node);
static int      node_has_type(xmlNode * node);
static int      node_has_attr(xmlNode * node, const xmlChar * att);
static void     add_multi_node_to_cv(xmlNode * node, swish_ConfigValue * cv);

static int meta_abbrev_len = 625;   /* 26 * 26 - 1 */
static char * meta_abbrev[] = {
"Aa", "Ab", "Ac", "Ad", "Ae", "Af", "Ag", "Ah", "Ai", "Aj", "Ak", "Al", "Am", 
"An", "Ao", "Ap", "Aq", "Ar", "As", "At", "Au", "Av", "Aw", "Ax", "Ay", "Az", 
"Ba", "Bb", "Bc", "Bd", "Be", "Bf", "Bg", "Bh", "Bi", "Bj", "Bk", "Bl", "Bm", 
"Bn", "Bo", "Bp", "Bq", "Br", "Bs", "Bt", "Bu", "Bv", "Bw", "Bx", "By", "Bz", 
"Ca", "Cb", "Cc", "Cd", "Ce", "Cf", "Cg", "Ch", "Ci", "Cj", "Ck", "Cl", "Cm", 
"Cn", "Co", "Cp", "Cq", "Cr", "Cs", "Ct", "Cu", "Cv", "Cw", "Cx", "Cy", "Cz", 
"Da", "Db", "Dc", "Dd", "De", "Df", "Dg", "Dh", "Di", "Dj", "Dk", "Dl", "Dm", 
"Dn", "Do", "Dp", "Dq", "Dr", "Ds", "Dt", "Du", "Dv", "Dw", "Dx", "Dy", "Dz", 
"Ea", "Eb", "Ec", "Ed", "Ee", "Ef", "Eg", "Eh", "Ei", "Ej", "Ek", "El", "Em", 
"En", "Eo", "Ep", "Eq", "Er", "Es", "Et", "Eu", "Ev", "Ew", "Ex", "Ey", "Ez", 
"Fa", "Fb", "Fc", "Fd", "Fe", "Ff", "Fg", "Fh", "Fi", "Fj", "Fk", "Fl", "Fm", 
"Fn", "Fo", "Fp", "Fq", "Fr", "Fs", "Ft", "Fu", "Fv", "Fw", "Fx", "Fy", "Fz", 
"Ga", "Gb", "Gc", "Gd", "Ge", "Gf", "Gg", "Gh", "Gi", "Gj", "Gk", "Gl", "Gm", 
"Gn", "Go", "Gp", "Gq", "Gr", "Gs", "Gt", "Gu", "Gv", "Gw", "Gx", "Gy", "Gz", 
"Ha", "Hb", "Hc", "Hd", "He", "Hf", "Hg", "Hh", "Hi", "Hj", "Hk", "Hl", "Hm", 
"Hn", "Ho", "Hp", "Hq", "Hr", "Hs", "Ht", "Hu", "Hv", "Hw", "Hx", "Hy", "Hz", 
"Ia", "Ib", "Ic", "Id", "Ie", "If", "Ig", "Ih", "Ii", "Ij", "Ik", "Il", "Im", 
"In", "Io", "Ip", "Iq", "Ir", "Is", "It", "Iu", "Iv", "Iw", "Ix", "Iy", "Iz", 
"Ja", "Jb", "Jc", "Jd", "Je", "Jf", "Jg", "Jh", "Ji", "Jj", "Jk", "Jl", "Jm", 
"Jn", "Jo", "Jp", "Jq", "Jr", "Js", "Jt", "Ju", "Jv", "Jw", "Jx", "Jy", "Jz", 
"Ka", "Kb", "Kc", "Kd", "Ke", "Kf", "Kg", "Kh", "Ki", "Kj", "Kk", "Kl", "Km", 
"Kn", "Ko", "Kp", "Kq", "Kr", "Ks", "Kt", "Ku", "Kv", "Kw", "Kx", "Ky", "Kz", 
"La", "Lb", "Lc", "Ld", "Le", "Lf", "Lg", "Lh", "Li", "Lj", "Lk", "Ll", "Lm", 
"Ln", "Lo", "Lp", "Lq", "Lr", "Ls", "Lt", "Lu", "Lv", "Lw", "Lx", "Ly", "Lz", 
"Ma", "Mb", "Mc", "Md", "Me", "Mf", "Mg", "Mh", "Mi", "Mj", "Mk", "Ml", "Mm", 
"Mn", "Mo", "Mp", "Mq", "Mr", "Ms", "Mt", "Mu", "Mv", "Mw", "Mx", "My", "Mz", 
"Na", "Nb", "Nc", "Nd", "Ne", "Nf", "Ng", "Nh", "Ni", "Nj", "Nk", "Nl", "Nm", 
"Nn", "No", "Np", "Nq", "Nr", "Ns", "Nt", "Nu", "Nv", "Nw", "Nx", "Ny", "Nz", 
"Oa", "Ob", "Oc", "Od", "Oe", "Of", "Og", "Oh", "Oi", "Oj", "Ok", "Ol", "Om", 
"On", "Oo", "Op", "Oq", "Or", "Os", "Ot", "Ou", "Ov", "Ow", "Ox", "Oy", "Oz", 
"Pa", "Pb", "Pc", "Pd", "Pe", "Pf", "Pg", "Ph", "Pi", "Pj", "Pk", "Pl", "Pm", 
"Pn", "Po", "Pp", "Pq", "Pr", "Ps", "Pt", "Pu", "Pv", "Pw", "Px", "Py", "Pz", 
"Qa", "Qb", "Qc", "Qd", "Qe", "Qf", "Qg", "Qh", "Qi", "Qj", "Qk", "Ql", "Qm", 
"Qn", "Qo", "Qp", "Qq", "Qr", "Qs", "Qt", "Qu", "Qv", "Qw", "Qx", "Qy", "Qz", 
"Ra", "Rb", "Rc", "Rd", "Re", "Rf", "Rg", "Rh", "Ri", "Rj", "Rk", "Rl", "Rm", 
"Rn", "Ro", "Rp", "Rq", "Rr", "Rs", "Rt", "Ru", "Rv", "Rw", "Rx", "Ry", "Rz", 
"Sa", "Sb", "Sc", "Sd", "Se", "Sf", "Sg", "Sh", "Si", "Sj", "Sk", "Sl", "Sm", 
"Sn", "So", "Sp", "Sq", "Sr", "Ss", "St", "Su", "Sv", "Sw", "Sx", "Sy", "Sz", 
"Ta", "Tb", "Tc", "Td", "Te", "Tf", "Tg", "Th", "Ti", "Tj", "Tk", "Tl", "Tm", 
"Tn", "To", "Tp", "Tq", "Tr", "Ts", "Tt", "Tu", "Tv", "Tw", "Tx", "Ty", "Tz", 
"Ua", "Ub", "Uc", "Ud", "Ue", "Uf", "Ug", "Uh", "Ui", "Uj", "Uk", "Ul", "Um", 
"Un", "Uo", "Up", "Uq", "Ur", "Us", "Ut", "Uu", "Uv", "Uw", "Ux", "Uy", "Uz", 
"Va", "Vb", "Vc", "Vd", "Ve", "Vf", "Vg", "Vh", "Vi", "Vj", "Vk", "Vl", "Vm", 
"Vn", "Vo", "Vp", "Vq", "Vr", "Vs", "Vt", "Vu", "Vv", "Vw", "Vx", "Vy", "Vz", 
"Wa", "Wb", "Wc", "Wd", "We", "Wf", "Wg", "Wh", "Wi", "Wj", "Wk", "Wl", "Wm", 
"Wn", "Wo", "Wp", "Wq", "Wr", "Ws", "Wt", "Wu", "Wv", "Ww", "Wx", "Wy", "Wz", 
"Xa", "Xb", "Xc", "Xd", "Xe", "Xf", "Xg", "Xh", "Xi", "Xj", "Xk", "Xl", "Xm", 
"Xn", "Xo", "Xp", "Xq", "Xr", "Xs", "Xt", "Xu", "Xv", "Xw", "Xx", "Xy", "Xz", 
"Ya", "Yb", "Yc", "Yd", "Ye", "Yf", "Yg", "Yh", "Yi", "Yj", "Yk", "Yl", "Ym", 
"Yn", "Yo", "Yp", "Yq", "Yr", "Ys", "Yt", "Yu", "Yv", "Yw", "Yx", "Yy", "Yz", 
"Za", "Zb", "Zc", "Zd", "Ze", "Zf", "Zg", "Zh", "Zi", "Zj", "Zk", "Zl", "Zm", 
"Zn", "Zo", "Zp", "Zq", "Zr", "Zs", "Zt", "Zu", "Zv", "Zw", "Zx", "Zy", "Zz" 
};


static void
free_config2(void *payload, xmlChar * key)
{
    if (SWISH_DEBUG > 10)
        swish_debug_msg("   freeing config %s => %s", key, (xmlChar *) payload);

    swish_xfree((xmlChar *) payload);
}

static void
free_config1(void *payload, xmlChar * confName)
{
    int             size = xmlHashSize((xmlHashTablePtr) payload);

    if (SWISH_DEBUG > 10)
    {
        swish_debug_msg(" freeing config %s =>", confName);
        swish_debug_msg(" num of keys in config hash: %d", size);
        swish_debug_msg(" ptr addr: 0x%x  %d", (int) payload, (int) payload);
    }

    xmlHashFree((xmlHashTablePtr) payload, (xmlHashDeallocator) free_config2);

}

void
swish_free_config(swish_Config * config)
{
    int             size = xmlHashSize(config->conf);

    if (SWISH_DEBUG > 10)
    {
        swish_debug_msg("freeing config");
        swish_debug_msg("num of keys in config hash: %d", size);
        swish_debug_msg("ptr addr: 0x%x  %d", (int) config, (int) config);
    }

    xmlHashFree(config->conf, (xmlHashDeallocator) free_config1);

    if (config->stash != NULL)
    {
        swish_warn_err("possible memory leak: config->stash was not freed");
    }

    swish_xfree(config);
}



/* init memory stuff, get global debug and env vars, and verify locale is correct */

swish_Config  *
swish_init_config()
{

    /* declare all our vars */
    swish_Config  *config;
    xmlHashTablePtr c, metas, parsers, index, prop, alias, parsewords;


    /* init memory */
    swish_init_memory();

    /* verify locale */
    swish_verify_utf8_locale();

    /* set global vars */
    setenv("SWISH_DEBUG", "0", 0);
    SWISH_DEBUG = strtol(getenv("SWISH_DEBUG"), (char **) NULL, 10);

    if (SWISH_DEBUG)
        swish_debug_msg("creating default config");

    /* create our object */

    config = swish_xmalloc(sizeof(swish_Config));

    /* init all config hashes */

    c = xmlHashCreate(16);

    /* in order to consistently free our hashes, we strdup everything */

    /* default metanames */
    metas = xmlHashCreate(8);
    swish_hash_add(metas,
            (xmlChar *) SWISH_DEFAULT_METANAME,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_VALUE));
    swish_hash_add(metas,
            (xmlChar *) SWISH_TITLE_METANAME,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_VALUE));
    swish_hash_add(c, (xmlChar *) SWISH_META, metas);


    /* default MIME types */
    swish_hash_add(c, (xmlChar *) SWISH_MIME, swish_mime_hash());


    /* default parser types - others added via config files */
    parsers = xmlHashCreate(5);
    swish_hash_add(parsers,
            (xmlChar *) "text/plain",
            swish_xstrdup((xmlChar *) SWISH_PARSER_TXT));
    swish_hash_add(parsers,
            (xmlChar *) "text/xml",
            swish_xstrdup((xmlChar *) SWISH_PARSER_XML));
    swish_hash_add(parsers,
            (xmlChar *) "text/html",
            swish_xstrdup((xmlChar *) SWISH_PARSER_HTML));
    swish_hash_add(parsers,
            (xmlChar *) SWISH_DEFAULT_PARSER,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_PARSER_TYPE));

    swish_hash_add(c, (xmlChar *) SWISH_PARSERS, parsers);


    /* index attributes -- while testing, define all available */
    index = xmlHashCreate(4);

    swish_hash_add(index,
            (xmlChar *) SWISH_INDEX_FORMAT,
            swish_xstrdup((xmlChar *) SWISH_INDEX_FILEFORMAT));
    swish_hash_add(index,
            (xmlChar *) SWISH_INDEX_NAME,
            swish_xstrdup((xmlChar *) SWISH_INDEX_FILENAME));
    swish_hash_add(index,
            (xmlChar *) SWISH_INDEX_LOCALE,
            swish_xstrdup((xmlChar *) setlocale(LC_ALL, NULL)));

    swish_hash_add(c, (xmlChar *) SWISH_INDEX, index);

    /* properties hash: each property is "propertyname" => type these match tag
     * (meta) names and are aggregated for each doc, in propHash */
    prop = xmlHashCreate(8);
    swish_hash_add(prop,
            (xmlChar *) SWISH_PROP_DESCRIPTION,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_VALUE));
    swish_hash_add(prop,
            (xmlChar *) SWISH_PROP_TITLE,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_VALUE));

    swish_hash_add(c, (xmlChar *) SWISH_PROP, prop);


    /* aliases: other names a tag might be known as, for matching properties and
     * metanames */
    alias = xmlHashCreate(8);

    swish_hash_add(alias,
            (xmlChar *) SWISH_TITLE_TAG,
            swish_xstrdup((xmlChar *) SWISH_TITLE_METANAME));
    swish_hash_add(alias,
            (xmlChar *) SWISH_BODY_TAG,
            swish_xstrdup((xmlChar *) SWISH_PROP_DESCRIPTION));

    swish_hash_add(c, (xmlChar *) SWISH_ALIAS, alias);

    /* word parsing settings */
    parsewords = xmlHashCreate(4);
    swish_hash_add(parsewords,
            (xmlChar *) SWISH_PARSE_WORDS,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_VALUE));

    swish_hash_add(c, (xmlChar *) SWISH_WORDS, parsewords);

    config->conf = c;
    config->ref_cnt = 1;
    config->stash = NULL;


    if (SWISH_DEBUG > 10)
        swish_debug_config(config);

    return config;
}

/* PUBLIC */
swish_Config  *
swish_add_config(xmlChar * conf, swish_Config * config)
{

    config = swish_parse_config(conf, config);

    if (SWISH_DEBUG)
        swish_debug_config(config);


    return config;

}

swish_ConfigValue *
swish_init_ConfigValue()
{
    swish_ConfigValue *cv;
    cv = swish_xmalloc(sizeof(swish_ConfigValue));
    cv->ref_cnt = 1;
    cv->multi = 0;
    cv->equal = 0;
    cv->key = NULL;
    cv->value = NULL;
    return cv;
}

void
swish_free_ConfigValue(swish_ConfigValue * cv)
{
    if (cv->key != NULL)
        swish_xfree(cv->key);

    if (cv->value != NULL)
        swish_xfree(cv->value);

    swish_xfree(cv);
}

swish_ConfigValue *
swish_keys(swish_Config * config,...)
{
    va_list         args;
    swish_ConfigValue *cv = swish_init_ConfigValue();

    va_start(args, config);


    va_end(args);
    return cv;
}

swish_ConfigValue *
swish_value(swish_Config * config, xmlChar * key,...)
{
    va_list         args;
    swish_ConfigValue *cv = swish_init_ConfigValue();

    va_start(args, key);


    va_end(args);
    return cv;
}


/* a xml node is multi if: - it is a predefined top-level key (MetaNames, PropertyNames,
 * etc.) - it has the multi attribute present (value is ignored) - has one or more spaces
 * in the value */
static int 
is_multi(xmlNode * node)
{
    xmlChar        *v;

    if (xmlHasProp(node, (const xmlChar *) "multi") != NULL)
        return 1;

    if (is_top_level(node))
        return 1;

    v = get_node_value(node);
    if (v != NULL && (xmlStrchr(v, ' ') || xmlStrchr(v, '\n')))
    {
        xmlFree(v);
        return 1;
    }

    return 0;
}

static int 
is_top_level(xmlNode * node)
{
    if (xmlStrEqual(node->name, (xmlChar *) SWISH_META))
        return 1;

    if (xmlStrEqual(node->name, (xmlChar *) SWISH_PROP))
        return 1;

    if (xmlStrEqual(node->name, (xmlChar *) SWISH_PROP_MAX))
        return 1;

    if (xmlStrEqual(node->name, (xmlChar *) SWISH_PROP_SORT))
        return 1;

    return 0;
}


/* a xml node is equal if: - no attributes are present, just content - 'key' attr is
 * present and xmlStrEqual() with content - 'key' and 'value' attrs are both present and
 * xmlStrEqual() */
static int 
is_equal(xmlNode * node)
{
    xmlChar        *v, *k;
    int             e;

    if (!node_has_key(node) && !node_has_value(node))
        return 1;

    v = get_node_value(node);
    k = xmlGetProp(node, (const xmlChar *) "key");
    e = xmlStrEqual(k, v);
    xmlFree(v);
    xmlFree(k);

    if (e)
        return 1;


    if (node_has_value(node) && !node_has_key(node))
    {
        swish_fatal_err("config node with value but no key: %s", node->name);
        return 0;
    }

    return 0;
}

static int 
node_has_key(xmlNode * node)
{
    return node_has_attr(node, (const xmlChar *) "key");
}

static int 
node_has_value(xmlNode * node)
{
    return node_has_attr(node, (const xmlChar *) "value");
}

static int 
node_has_type(xmlNode * node)
{
    return node_has_attr(node, (const xmlChar *) "type");
}

static int 
node_has_attr(xmlNode * node, const xmlChar * att)
{
    if (xmlHasProp(node, att))
        return 1;
    else
        return 0;
}



/* key is attr value if present, otherwise get_node_value() */
static xmlChar *
get_node_key(xmlNode * node)
{
    xmlChar        *k;
    k = xmlGetProp(node, (const xmlChar *) "key");
    return k;        /* MUST be freed */
}

static xmlChar *
get_node_type(xmlNode * node)
{
    xmlChar        *k;
    k = xmlGetProp(node, (const xmlChar *) "type");
    return k;        /* MUST be freed */
}

/* get node content and remove and leading or trailing spaces */
static xmlChar *
get_node_value(xmlNode * node)
{
    xmlChar        *str;
    str = xmlNodeGetContent(node);
    if (str == NULL)
    {
        str = xmlGetProp(node, (const xmlChar *) "value");
        if (str == NULL)
        {
            swish_warn_err("no value for config opt '%s'", node->name);
            return NULL;
        }
    }


    /* TODO strip spaces */

    return str;        /* MUST be freed */
}

static xmlChar *
get_node_name(xmlNode * node)
{
    return swish_xstrdup(node->name);
}

static xmlDocPtr 
parse_xml_config(xmlChar * conf)
{
    xmlDocPtr       doc;
    struct stat     fileinfo;

    /* parse either a filename, or, if we can't stat it, assume conf is a XML string */
    if (!stat((char *) conf, &fileinfo))
    {
        doc = xmlParseFile((const char *) conf);
        if (SWISH_DEBUG)
            swish_debug_msg("Parsing configuration file: %s", conf);
    }
    else
    {
        doc = xmlParseMemory((const char *) conf, xmlStrlen(conf));
        if (SWISH_DEBUG)
            swish_debug_msg("Parsing configuration from memory");
    }

    if (doc == NULL)
        swish_fatal_err("error: could not parse XML: %s", conf);

    return doc;

}

static void 
add_multi_node_to_cv(xmlNode * node, swish_ConfigValue * cv)
{
    xmlChar        *key, *tmp, *type;
    swish_StringList *value;
    int             i;

    /* if node has an explicit key, use that with entire value else split up value
     * into tokens on whitespace. if node has a 'type' attr, use that as hash value,
     * else use str as both key/value. */

    if (node_has_key(node))
    {
        key = get_node_key(node);
        tmp = get_node_value(node);
        if (xmlHashLookup(cv->value, key))
            swish_hash_replace(cv->value, key, tmp);
        else
            swish_hash_add(cv->value, key, tmp);

    }
    else
    {
        tmp = get_node_value(node);
        value = swish_make_StringList(tmp);
        xmlFree(tmp);

        if (node_has_type(node))
            type = get_node_type(node);


        for (i = 0; i < value->n; i++)
        {
            key = swish_xstrdup(value->word[i]);
            if (type == NULL)
                tmp = swish_xstrdup(key);
            else
                tmp = type;

            if (xmlHashLookup(cv->value, key))
                swish_hash_replace(cv->value, key, tmp);
            else
                swish_hash_add(cv->value, key, tmp);
        }
    }

}

static xmlNode *
get_dom_root(xmlDocPtr doc)
{
    xmlNode        *root = NULL;
    xmlChar        *toptag = "swishconfig";

    root = xmlDocGetRootElement(doc);

    /* --------------------------------------------------------------------------
     * Must have root element, a name and the name must be "swishconfig"
     * -------------------------------------------------------------------------- */
    if (!root ||
        !root->name ||
        !xmlStrEqual(root->name, toptag))
    {
        xmlFreeDoc(doc);
        swish_fatal_err("bad config format: malformed or missing '%s' toplevel tag", toptag);
        return 0;
    }

    return root;
}


/* this is too complicated right now. might be easier to just plunge in and define
 * different config opt types (MetaNames, etc.) rather than try and remain agnostic and
 * extensible about attribute names, etc. */
swish_Config  *
swish_parse_config_new(xmlChar * conf, swish_Config * config)
{

    xmlNode        *node, *root;
    xmlDocPtr       doc;
    swish_ConfigValue *cv;

    doc = parse_xml_config(conf);
    root = get_dom_root(doc);

    /* -------------------------------------------------------------------------- get
     * options/values format is:
     * 
     * <directive type="someval">some arg</directive>
     * 
     * where type="someval" attr is optional -- defaults to whatever 'some arg' is
     * -------------------------------------------------------------------------- */

    for (node = root->children; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {

            /* is this opt already in our config? if yes, and multi, append
             * it if yes, and !multi, replace it if no, create it */

            if (xmlHashLookup(config->conf, node->name))
            {
                cv = xmlHashLookup(config->conf, node->name);

                if (cv->multi)    /* already flagged as multi */
                {
                    swish_debug_msg("%s is an existing multi-config", node->name);

                    add_multi_node_to_cv(node, cv);

                }
                else
                {
                    swish_debug_msg("%s exists but is not a multi-config", node->name);

                    /* free the existing one and replace it with new
                     * one */
                    swish_free_ConfigValue(cv);

                    cv = swish_init_ConfigValue();
                    cv->key = get_node_name(node);
                    cv->value = get_node_value(node);

                }
            }
            else
            {
                /* TODO - split all these if/else into separate functions */
                cv = swish_init_ConfigValue();

                if (is_multi(node))
                {
                    swish_debug_msg("%s is a new multi-config", node->name);
                    cv->value = swish_new_hash(16);
                    add_multi_node_to_cv(node, cv);

                }

                if (is_equal(node))
                {
                    swish_debug_msg("%s is an equal node", node->name);
                    cv->equal = 1;
                }


            }

        }

    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return config;
}

swish_Config  *
swish_parse_config(xmlChar * conf, swish_Config * config)
{
    xmlNode        *cur_node, *root;
    xmlChar        *opt_name, *opt_type, *opt_arg, *tmp_arg, *tmp_value;
    int             name_seen, i, free_tmp;
    swish_StringList *arg_list;
    xmlHashTablePtr vhash;
    xmlDocPtr       doc;

    doc = parse_xml_config(conf);
    root = get_dom_root(doc);

    /* init all strings */

    opt_name = NULL;
    opt_type = NULL;
    opt_arg = NULL;


    /* -------------------------------------------------------------------------- get
     * options/values format is:
     * 
     * <directive type="someval">some arg</directive>
     * 
     * where type="someval" attr is optional -- defaults to whatever 'some arg' is
     * -------------------------------------------------------------------------- */

    for (cur_node = root->children; cur_node != NULL; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            /* fprintf(stderr, "Element: %s \n", cur_node->name); */

            opt_name = get_node_name(cur_node);

            opt_type = xmlGetProp(cur_node, (xmlChar *) "type");

            opt_arg = xmlNodeGetContent(cur_node);

            if (!opt_arg)
                swish_fatal_err("no value for option tag '%s'", opt_name);

            /* append value/args to any existing names in config */
            if (xmlHashLookup(config->conf, opt_name))
            {
                /* err(202,"Bad 'name' tag contents in config: already
                 * seen '%s'\n", opt_name); */
                vhash = xmlHashLookup(config->conf, opt_name);
                if (vhash == NULL)
                {
                    swish_fatal_err("error with existing name in config: %s", opt_name);
                }
                else
                {
                    if (SWISH_DEBUG)
                        swish_debug_msg(" >>> found existing name in config: %s", opt_name);

                    name_seen = 1;
                }

            }
            else
            {
                name_seen = 0;

                vhash = xmlHashCreate(16);    /* values => args */
                if (vhash == NULL)
                    swish_fatal_err("error creating vhash");

            }


            /* if we reference another config file, load it immediately but
             * don't load into hash, as we might have multiple config files
             * nested */
            if (xmlStrEqual(opt_name, (xmlChar *) SWISH_INCLUDE_FILE))
            {

                /* TODO deduce relative paths */
                swish_parse_config(opt_arg, config);
                swish_xfree(opt_name);
                xmlFree(opt_arg);
                continue;

            }

            /* split arg and value into words/phrases and add each one to
             * hash. This allows for multiple values in a single opt_name */


            arg_list = swish_make_StringList(opt_arg);

            for (i = 0; i < arg_list->n; i++)
            {

                free_tmp = 0;
                tmp_arg = arg_list->word[i];

                if (SWISH_DEBUG > 10)
                    swish_debug_msg("config %s tmp_arg = %s  opt_type = %s", opt_name, tmp_arg, opt_type);

                tmp_value = opt_type ? swish_xstrdup(opt_type) : swish_xstrdup(tmp_arg);

                /* normalize all metanames and properties since we
                 * compare against lc tag names */
                if (xmlStrEqual(opt_name, (xmlChar *) SWISH_META)
                    ||
                    xmlStrEqual(opt_name, (xmlChar *) SWISH_PROP)
                    ||
                    xmlStrEqual(opt_name, (xmlChar *) SWISH_PROP_MAX)
                    ||
                    xmlStrEqual(opt_name, (xmlChar *) SWISH_PROP_SORT)
                    )
                {
                    free_tmp = 1;
                    if (SWISH_DEBUG > 10)
                        swish_debug_msg("tolower str: >%s<", tmp_arg);

                    tmp_arg = swish_str_tolower(tmp_arg);

                }

                if (SWISH_DEBUG > 10)
                    swish_debug_msg("config %s tmp_arg = %s  tmp_value = %s", opt_name, tmp_arg, tmp_value);

                if (xmlHashLookup(vhash, tmp_arg))
                    swish_hash_replace(vhash, tmp_arg, tmp_value);
                else
                    swish_hash_add(vhash, tmp_arg, tmp_value);

                if (free_tmp)
                    swish_xfree(tmp_arg);


            }

            swish_free_StringList(arg_list);

            /* don't use our swish_xfree since it throws off memcount */
            xmlFree(opt_arg);

            if (opt_type != NULL)
                xmlFree(opt_type);


            /* add vhash to config hash unless we already have or it's just
             * calling another config file */
            if (!name_seen && xmlStrcmp(opt_name, (xmlChar *) "IncludeConfigFile"))
            {
                if (SWISH_DEBUG)
                    swish_debug_msg(" >>> adding %s to config hash ( name_seen = %d )", opt_name, name_seen);

                swish_hash_add(config->conf, opt_name, vhash);
            }


            swish_xfree(opt_name);

        }        /* end XML_ELEMENT_NODE */

    }            /* end cur_node */

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return config;
}


static void
config_val_printer(xmlChar * val, xmlChar * str, xmlChar * key)
{
    swish_debug_msg("   %s => %s", key, val);
}

static void
config_printer(xmlHashTablePtr vhash, xmlChar * str, xmlChar * key)
{
    swish_debug_msg(" Config %s:", key);

    xmlHashScan(vhash, (xmlHashScanner) config_val_printer, "vhash");

    return;
}

/* PUBLIC */
int
swish_debug_config(swish_Config * config)
{
    int             size = xmlHashSize(config->conf);

    swish_debug_msg("config->ref_cnt = %d", config->ref_cnt);
    swish_debug_msg("config->stash address = 0x%x  %d", (int) config->stash, (int) config->stash);
    swish_debug_msg("num of keys in config hash: %d", size);
    swish_debug_msg("ptr addr: 0x%x  %d", (int) config->conf, (int) config->conf);

    xmlHashScan(config->conf, (xmlHashScanner) config_printer, "opt name");

    return 1;

}

/* PUBLIC */
xmlHashTablePtr
swish_subconfig_hash(swish_Config * config, xmlChar * key)
{
    xmlHashTablePtr v = xmlHashLookup(config->conf, key);

    if (v == NULL)
    {
        /* why does this happen when value is a hashptr ? */
        swish_debug_msg("Config option '%s' has NULL value", key);
    }

    return v;
}

/* PUBLIC */
/* returns true/false if 'value' is a valid key in the subconfig hash for 'key' */
/* example might be looking up a specific metaname in the MetaNames hash */
int
swish_config_value_exists(swish_Config * config, xmlChar * key, xmlChar * value)
{
    xmlHashTablePtr v = swish_subconfig_hash(config, key);
    return ((int) xmlHashLookup(v, value));
}

xmlChar        *
swish_get_config_value(swish_Config * config, xmlChar * key, xmlChar * value)
{
    xmlHashTablePtr v = swish_subconfig_hash(config, key);
    return (xmlChar *) xmlHashLookup(v, value);
}
