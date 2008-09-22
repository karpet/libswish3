use Test::More tests => 3;

use_ok('SWISH::3');
ok(my $a = SWISH::3->new->analyzer,   'new analyzer');
is($a->refcount, 1,                   'refcount');
