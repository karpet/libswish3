use Test::More tests => 22;
use Carp;
#use Devel::Peek;
#use Data::Dump qw( dump );

#use Devel::Monitor qw(:all);

BEGIN
{
    use_ok('SWISH::3::Parser');
}

ok(
    my $parser =
      SWISH::3::Parser->new(
              config => '<swishconfig><MetaNames>foo</MetaNames></swishconfig>',
              handler => \&getmeta
      ),
    "new parser"
  );

my $r = 0;
while ($r < 10)
{
    ok($r += $parser->parse_file("t/test.html"), "parse HTML");

    #diag("r = $r");
}
$r = 0;
while ($r < 10)
{
    ok($r += $parser->parse_file("t/test.xml"), "parse XML");

    #diag("r = $r");
}

sub getmeta
{
    my $data = shift;

#    diag(dump($data->metanames));
#   $data->wordlist->debug;

}
