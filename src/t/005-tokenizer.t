#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 12;
use SwishTestUtils;

$ENV{SWISH_DEBUG_TOKENIZER} = 1;

ok( my $buf = SwishTestUtils::run_get_stderr('./swish_tokenize foobar'),
    "tokenize foobar" );

#diag($buf);

like( $buf, qr/t->len\s+= 6/,        'length 6' );
like( $buf, qr/t->value\s+= foobar/, 'value foobar' );

ok( $buf = SwishTestUtils::run_get_stderr('./swish_tokenize ++foo++'),
    "tokenize ++foo++" );

#diag($buf);

like( $buf, qr/t->len\s+= 3/,     'length 3' );
like( $buf, qr/t->value\s+= foo/, 'value foo' );

ok( $buf = SwishTestUtils::run_get_stderr(
        './swish_tokenize 布朗在迅速跳下懒狐狗'),
    "tokenize chinese"
);

#diag($buf);

like( $buf, qr/parsed 1 tokens/, "1 token" );

ok( $buf = SwishTestUtils::run_get_stderr(
        "./swish_tokenize 'el zorro marrón rápido saltó sobre el perro perezoso'"
    ),
    "tokenize spanish"
);

#diag($buf);

like( $buf, qr/parsed 9 tokens/, "9 tokens" );

ok( $buf = SwishTestUtils::run_get_stderr(
        "./swish_tokenize 'http://FOOBAR.COM/'"
    ),
    "tokenize uri"
);

#diag($buf);

like( $buf, qr/parsed 3 tokens/, "3 tokens" );
