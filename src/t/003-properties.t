#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 4;
use SwishTestUtils;

$ENV{SWISH_DEBUG_NAMEDBUFFER} = 1;

my $buf = SwishTestUtils::run_lint_stderr('properties.html');

like( $buf, qr!<swishtitle>properties test page title</swishtitle>!,
    "swishtitle" );

like(
    $buf,
    qr!<swishdescription>properties test page body\s+a bunch of space between</swishdescription>!,
    "swishdescription"
);

$buf = SwishTestUtils::run_lint_stderr( 'dom.xml', 'dom.conf' );

# the dot . is the TOKENPOS_BUMPER delimiter
like( $buf, qr!<doc\.one\.two>green.yellow</doc\.one\.two>!, "dom" );

#diag($buf);

$buf = SwishTestUtils::run_lint_stderr( 'props.xml', 'props.conf' );

#diag( $buf );

like( $buf, qr/Property:<prop1>“foo<\/prop1>/, "no space after utf8 in property");
