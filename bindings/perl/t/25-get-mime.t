#!/usr/bin/env perl
use strict;
use warnings;
use Test::More tests => 9;

use_ok('SWISH::3');

ok( my $s3 = SWISH::3->new(), "new swish3" );
is( $s3->get_mime('foo/bar'),     undef,        "file with no ext is undef" );
is( $s3->get_mime('foo/bar.txt'), 'text/plain', "file.txt is text/plain" );
is( $s3->get_real_mime('foo/bar.txt'),
    'text/plain', "file.txt real mime is text/plain" );
is( $s3->get_mime('foo/bar.txt.gz'),
    'application/x-gzip', "file.txt.gz is application/x-gzip" );
is( $s3->get_real_mime('foo/bar.txt.gz'),
    'text/plain', "file.txt.gz real mime is text/plain" );
is( $s3->looks_like_gz('foo/bar.txt.gz'), 1, "file.txt.gz looks like gz" );
is( $s3->looks_like_gz('foo/bar.txt'), 0, "file.txt does not look like gz" );

