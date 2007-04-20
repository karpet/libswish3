use Test::More tests => 5;
use Carp;

#use Devel::Peek;
#use Devel::Monitor qw(:all);

BEGIN
{
    use_ok('SWISH::3::Analyzer');
    use_ok('SWISH::3::Constants');
}

ok(my $analyzer = SWISH::3::Analyzer->new(), "new tokenizer");

ok(
    my $wlist =
      $analyzer->tokenize(
                         "now is the time, ain't it? or when else might it be!",
                         13, 14, 'foo', 'bar'
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
