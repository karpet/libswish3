use Test::More tests => 3;

use Devel::Peek;

BEGIN { use_ok('SWISH::3') };

ok( my $parser = SWISH::3->new,   "new object");

ok( my $v = $parser->libxml2_version,   "libxml2 version");
diag($v);

#Dump $v;
