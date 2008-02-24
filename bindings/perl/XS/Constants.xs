# TODO more from libswish3.h               
# constants           
BOOT:
        {
        HV *stash;
  
        stash = gv_stashpv("SWISH::3",       TRUE);
        newCONSTSUB(stash, "SWISH_PROP",           newSVpv(SWISH_PROP, 0));
        newCONSTSUB(stash, "SWISH_PROP_MAX",       newSVpv(SWISH_PROP_MAX, 0));
        newCONSTSUB(stash, "SWISH_PROP_SORT",      newSVpv(SWISH_PROP_SORT, 0));
        newCONSTSUB(stash, "SWISH_META",           newSVpv(SWISH_META, 0));
        newCONSTSUB(stash, "SWISH_MIME",           newSVpv(SWISH_MIME, 0));
        newCONSTSUB(stash, "SWISH_PARSERS",        newSVpv(SWISH_PARSERS, 0));
        newCONSTSUB(stash, "SWISH_INDEX",          newSVpv(SWISH_INDEX, 0));
        newCONSTSUB(stash, "SWISH_ALIAS",          newSVpv(SWISH_ALIAS, 0));
        newCONSTSUB(stash, "SWISH_WORDS",          newSVpv(SWISH_WORDS, 0));
        newCONSTSUB(stash, "SWISH_PROP_RECCNT",    newSVpv(SWISH_PROP_RECCNT, 0));
        newCONSTSUB(stash, "SWISH_PROP_RANK",      newSVpv(SWISH_PROP_RANK, 0));
        newCONSTSUB(stash, "SWISH_PROP_DOCID",     newSVpv(SWISH_PROP_DOCID, 0));
        newCONSTSUB(stash, "SWISH_PROP_DOCPATH",   newSVpv(SWISH_PROP_DOCPATH, 0));
        newCONSTSUB(stash, "SWISH_PROP_DBFILE",    newSVpv(SWISH_PROP_DBFILE, 0));
        newCONSTSUB(stash, "SWISH_PROP_TITLE",     newSVpv(SWISH_PROP_TITLE, 0));
        newCONSTSUB(stash, "SWISH_PROP_SIZE",      newSVpv(SWISH_PROP_SIZE, 0));
        newCONSTSUB(stash, "SWISH_PROP_MTIME",     newSVpv(SWISH_PROP_MTIME, 0));
        newCONSTSUB(stash, "SWISH_PROP_DESCRIPTION",newSVpv(SWISH_PROP_DESCRIPTION, 0));
        newCONSTSUB(stash, "SWISH_PROP_CONNECTOR", newSVpv(SWISH_PROP_CONNECTOR, 0));
        newCONSTSUB(stash, "SWISH_PROP_STRING",    newSViv(SWISH_PROP_STRING));
        newCONSTSUB(stash, "SWISH_PROP_DATE",      newSViv(SWISH_PROP_DATE));
        newCONSTSUB(stash, "SWISH_PROP_INT",       newSViv(SWISH_PROP_INT));
        }
