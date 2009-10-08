#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 2;
use SwishTestUtils;

my $topdir = $ENV{SVNDIR} || '..';
my $test_configs = "$topdir/src/test_configs";

# test header read/write
ok( my $config = SwishTestUtils::slurp("$test_configs/swish.xml"),
    "slurp test_configs/swish.xml" );
system("./swish_header $test_configs/swish.xml")
    and die "can't run swish_header: $!";
ok( my $header = SwishTestUtils::slurp('swish_header.xml'),
    "slurp swish_header.xml" );

# TODO find specific patterns to compare in $config and $header

