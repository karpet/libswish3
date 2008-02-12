use Test::More tests => 3;

use_ok('SWISH::3');

ok( my $s3 = SWISH::3->new(), "new object" );

ok( my $buf = $s3->slurp("t/test.html"), "slurp file" );

#diag( $s3->dump );
#diag( $s3->refcount );

#diag($buf);

