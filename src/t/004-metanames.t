#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 10;
use SwishTestUtils;

$ENV{SWISH_DEBUG_NAMEDBUFFER} = 1;

ok( my $buf
        = SwishTestUtils::run_get_stderr( 'properties.html',
        'properties.xml' ),
    "properties.html"
);

like( $buf, qr!<swishtitle>properties test page title</swishtitle>!,
    "swishtitle" );

like( $buf, qr!<meta1> substr: properties test page meta1!, "first meta1" );
like( $buf, qr!<meta1> substr: more meta1!, "second meta1" );

#diag($buf);

ok( $buf
        = SwishTestUtils::run_get_stderr( 'UPPERlower.XML',
        'UPPERlower.XML' ),
    "UPPERlower.XML"
);

like( $buf, qr!<swishtitle>mytitle here</swishtitle>!,       "swishtitle" );
like( $buf, qr!<mytag1> substr: \s+ some text!s,             "mytag1" );
like( $buf, qr!<mytag1> substr: \s+ yet again\s+and again!s, "mytag1 again" );
like( $buf, qr!<mytag3.foo> substr: \s+ blah blah!s,         "mytag3.foo" );
like( $buf, qr!<mytag3> substr: \s+ foo bar!s,               "mytag3" );
