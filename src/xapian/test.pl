#!/usr/bin/perl
use strict;
use warnings;
use Carp;
use Test::More tests => 5;

my $topdir = $ENV{SVNDIR} || '..';

ok( run(''), 'usage' );

# indexing
ok( run(" $topdir/test_docs/*xml"),  'index xml' );
ok( run(" $topdir/test_docs/*html"), 'index html' );

# searching
ok( ( grep {m/2 estimated matches/} run(' --query swishtitle:foobar') ),
    'search swishtitle:foobar' );

# deleting
ok( run(" --Delete $topdir/test_docs/*xml"), "delete test_docs/*xml");

sub run {
    my $cmd = shift;
    my @out = `./swish_xapian $cmd`;
    if ($?) {
        croak "$cmd failed: $!";
    }
    return @out;
}
