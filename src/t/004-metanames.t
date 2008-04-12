#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 4;
use SwishTestUtils;

$ENV{SWISH_DEBUG_NAMEDBUFFER} = 1;

my $buf
    = SwishTestUtils::run_get_stderr( 'properties.html', 'properties.xml' );

like( $buf, qr!<swishtitle>properties test page title</swishtitle>!,
    "swishtitle" );

like( $buf, qr!<swishdefault> substr: properties test page title\n!,
    "title" );
like( $buf, qr!<swishdefault> substr: properties test page meta1\n!,
    "meta1" );

like( $buf, qr!<meta1>properties test page meta1</meta1>!, "meta1" );

#diag($buf);
