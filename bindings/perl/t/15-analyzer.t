use Test::More tests => 5;

use SWISH::3;

ok( my $s3 = SWISH::3->new(), "new s3 object" );

#ok( my $analyzer = $s3->analyzer, "get analyzer" );

eval { my $handler = $s3->analyzer->get_token_handler };

ok( $@, "get token handler: $@" );

like( 'foo', $s3->analyzer->get_regex, 'get regex' );

ok( !$s3->analyzer->set_token_handler( sub { $_[0]->debug } ),
    "set token handler" );

ok( $s3->tokenize('foo bar baz'), "tokenize" );

