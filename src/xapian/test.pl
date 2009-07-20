#!/usr/bin/perl

use Carp;
use Test::More tests => 5;

ok( run(''), 'usage' );

# indexing
ok( run(' ../test_docs/*xml'),  'index xml' );
ok( run(' ../test_docs/*html'), 'index html' );

# searching
ok( ( grep {m/2 estimated matches/} run(' --query swishtitle:foobar') ),
    'search swishtitle:foobar' );

# deleting
ok( run(' --Delete ../test_docs/*xml'), "delete test_docs/*xml");

sub run {
    my $cmd = shift;
    my @out = `./swish_xapian $cmd`;
    if ($?) {
        croak "$cmd failed: $!";
    }
    return @out;
}
