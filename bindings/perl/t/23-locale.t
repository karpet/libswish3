#!/usr/bin/env perl
use strict;
use Test::More tests => 1;
use SWISH::3;

BEGIN {
    use POSIX qw(locale_h);
    use locale;
    setlocale( LC_ALL, 'en_US.UTF-8' );
}

my $locale_ctype = setlocale(LC_CTYPE);
diag("setlocale(LC_CTYPE) = $locale_ctype");
my $locale_all = setlocale(LC_ALL);
diag("setlocale(LC_ALL) = $locale_all");

is( $locale_ctype, 'en_US.UTF-8', "Perl setlocale() works" );
if ( $locale_ctype !~ m/UTF-8/ ) {
    warn `locale -a`;
}

# test mbs support
diag("testing multibyte string support");
SWISH::3->wc_report(1000);
