use Test::More tests => 202;
use Carp;
use Devel::Peek;

#use Devel::Monitor qw(:all);

BEGIN
{
    use_ok('SWISH::3');
}

ok(my $parser = SWISH::3::Parser->new(handler => sub {  }),
    "new parser");

#monitor('parser' => \$parser);

#carp "test 3:";
#Dump $parser;
#Dump $parser->config;
my $r = 0;
while ($r < 100)
{
    ok($r += $parser->parse_file("t/test.html"), "parse HTML");

    #diag("r = $r");
}
while ($r < 200)
{
    ok($r += $parser->parse_file("t/test.xml"), "parse XML");

    #diag("r = $r");
}
