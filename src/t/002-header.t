#!/usr/bin/env perl
use strict;
use warnings;
use Test::More tests => 6;
use SwishTestUtils;

my $topdir = $ENV{SVNDIR} || '..';
my $test_configs = "$topdir/src/test_configs";

# test header read/write
ok( my $config = SwishTestUtils::slurp("$test_configs/swish.xml"),
    "slurp test_configs/swish.xml" );
ok( my $basic = SwishTestUtils::run_get_stderr(
        "./swish_header $test_configs/swish.xml"),
    "basic swish_header usage"
);
ok( my $header = SwishTestUtils::slurp('swish_header.xml'),
    "slurp swish_header.xml" );

# TODO find specific patterns to compare in $config and $header

# xmlns
ok( my $xmlns = SwishTestUtils::run_get_stderr(
        "SWISH_DEBUG=16 ./swish_header --test $test_configs/xmlns.xml"),
    "test xmlns"
);

#diag($xmlns);
like( $xmlns, qr/m->name\ += swish:color/, "swish: xmlns" );
like( $xmlns, qr/m->name\ += foo:name/,    "foo: xmlns" );
