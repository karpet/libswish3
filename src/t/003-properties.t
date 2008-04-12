#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 2;
use SwishTestUtils;

$ENV{SWISH_DEBUG_NAMEDBUFFER} = 1;

my $buf = SwishTestUtils::run_get_stderr('properties.html');

like( $buf, qr!<swishtitle>properties test page title</swishtitle>!,
    "swishtitle" );

like(
    $buf,
    qr!<swishdescription>properties test page body\s+a bunch of space between</swishdescription>!,
    "swishdescription"
);

#diag($buf);
