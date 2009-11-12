#!/usr/bin/perl
use strict;
use warnings;
use Carp;
use Test::More tests => 5;
use FindBin;

my $testdir = $ENV{SVNDIR} || "$FindBin::Bin/..";

ok( run(''), 'usage' );

# indexing
ok( run(" $testdir/test_docs/*xml"),  'index xml' );
ok( run(" $testdir/test_docs/*html"), 'index html' );

# searching
ok( ( grep {m/2 estimated total matches/} run(' --query swishtitle:foobar') ),
    'search swishtitle:foobar'
);

# deleting
ok( run(" --Delete $testdir/test_docs/*.html"), "delete test_docs/*.html" );

sub run {
    my $cmd = shift;
    my @out = `./swish_xapian --config sx-conf.xml $cmd`;
    if ($?) {
        croak "$cmd failed: $!";
    }
    return @out;
}
