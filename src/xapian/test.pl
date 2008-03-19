#!/usr/bin/perl

use Carp;
use Test::More tests => 3;

ok( run(''),                    'usage' );
ok( run(' ../test_docs/*xml'),  'index xml' );
ok( run(' ../test_docs/*html'), 'index html' );

sub run {
    my $cmd = shift;
    my @out = `./swish_xapian $cmd`;
    if ($?) {
        croak "$cmd failed: $!";
    }
    return @out;
}
