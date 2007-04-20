use Test::More tests => 3;

use Devel::Peek;

use_ok('SWISH::3::Parser');

ok(my $parser = SWISH::3::Parser->new(handler => sub { }), "new object");

ok(my $buf = $parser->slurp_file("t/test.html"), "slurp file");

#diag($buf);

#Dump $parser;
