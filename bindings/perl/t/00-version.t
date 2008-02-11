use Test::More tests => 1;
BEGIN { use_ok('SWISH::3') };

diag( SWISH::3->version );
