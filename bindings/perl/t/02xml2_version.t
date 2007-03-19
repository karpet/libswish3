use Test::More tests => 4;

use Devel::Peek;

BEGIN { use_ok('SWISH::3') };

ok( my $x = SWISH::3->xml2_version,   "libxml2 version");
diag($x);

ok( my $s = SWISH::3->swish_version,   "swish version");
diag($s);

ok( my $l = SWISH::3->libswish3_version, "libswish3 version");
diag($l);

#Dump $v;
