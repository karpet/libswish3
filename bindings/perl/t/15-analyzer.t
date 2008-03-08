use Test::More tests => 4;

use SWISH::3;

ok( my $s3 = SWISH::3->new(), "new s3 object" );

ok( my $analyzer = $s3->analyzer, "get analyzer" );

eval { my $handler = $analyzer->get_token_handler };

ok( $@, "get token handler" );

like( 'foo', $analyzer->get_regex, 'get regex' );
