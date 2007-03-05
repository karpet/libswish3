use Test::More tests => 102;
use Carp;
use Devel::Peek;
#use Devel::Monitor qw(:all);

BEGIN
{
    use_ok('SWISH::3');
}

ok(my $parser = SWISH::3::Parser->new, "new parser");

#monitor('parser' => \$parser);

#carp "test 3:";
#Dump $parser;

#Dump $parser->config;
my $r = 0;
while ($r < 100)
{
    ok($r += $parser->parse_file("t/latin1.xml"), "parse latin1 XML");
    #diag("r = $r");
}
