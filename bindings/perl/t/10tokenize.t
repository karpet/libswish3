use Test::More tests => 5;
use Carp;

#use Devel::Peek;
#use Devel::Monitor qw(:all);

BEGIN
{
    use_ok('SWISH::3::Parser');
    use_ok('SWISH::3::Constants');
}

ok(my $swish3 = SWISH::3::Parser->new(handler => sub { }), "new swish3");

ok(
    my $wlist =
      $swish3->tokenize(
                        "now is the time, ain't it? or when else might it be!",
                        'foo', 'bar', 100, 1, 13, 14
                       ),
    "wordlist"
  );

ok($wlist->isa('SWISH::3::Parser::WordList'), 'isa wordlist');

while (my $swishword = $wlist->next)
{
    #diag('=' x 60);
    for my $w (SWISH_WORD_FIELDS)
    {

       # diag(sprintf("%15s: %s\n", $w, $swishword->$w));
    }
}
