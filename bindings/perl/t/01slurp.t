use Test::More tests => 3;

use Devel::Peek;

BEGIN { use_ok('SWISH::3') };

ok( my $parser = SWISH::3->new,   "new object");

ok( my $buf = $parser->slurp_file("t/test.html"),   "slurp file");
#diag($buf);

#Dump $buf;
