#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 2;
use SwishTestUtils;

# test header read/write
ok( my $config = SwishTestUtils::slurp('example/swish.xml'),
    "slurp example/swish.xml" );
system("./swish_header example/swish.xml")
    and die "can't run swish_header: $!";
ok( my $header = SwishTestUtils::slurp('swish_header.xml'),
    "slurp swish_header.xml" );

# TODO find specific patterns to compare in $config and $header

