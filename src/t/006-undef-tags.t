#!/usr/bin/env perl
use strict;
use warnings;
use Test::More tests => 6;
use lib 't';
use SwishTestUtils;

$ENV{SWISH_DEBUG_NAMEDBUFFER} = 1;

# ignore
my $buf = SwishTestUtils::run_lint_stderr( 'undeftags.xml',
    'undeftags-ignore.conf' );

#diag($buf);
like( $buf, qr(<swishdefault></swishdefault>), "ignore" );

# auto
$buf = SwishTestUtils::run_lint_stderr( 'undeftags.xml',
    'undeftags-auto.conf' );

#diag("AUTO:");
#diag($buf);
like( $buf, qr(<foo>bar</foo>),          "undef tags auto detected" );
like( $buf, qr(<name>John Smith</name>), "undef tags auto detected" );
like( $buf, qr(<name.age>23</name.age>), "undef xml attrs auto detected" );

# error
$buf = SwishTestUtils::run_lint_stderr( 'undeftags.xml',
    'undeftags-error.conf' );

like(
    $buf,
    qr(XML tag 'doc' is not a defined MetaName),
    "fatal error on undefined tag"
);

#diag($buf);

# index
$buf = SwishTestUtils::run_lint_stderr( 'undeftags.xml',
    'undeftags-index.conf' );

#diag($buf);
like( $buf, qr(<swishdefault>.+23.John Smith.+bar)s, "index" );
