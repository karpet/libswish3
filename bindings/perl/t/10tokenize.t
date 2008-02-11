use Test::More tests => 8;

use_ok('SWISH::3');
use_ok('SWISH::3::Constants');

ok( my $analyzer = SWISH::3->new()->get_analyzer, "new tokenizer" );

ok( my $wlist = $analyzer->tokenize(
        "now is the time, ain't it? or when else might it be!",
        13, 14, 'foo', 'bar'
    ),
    "wordlist"
);

ok( $wlist->isa('SWISH::3::WordList'), 'isa wordlist' );

while ( my $swishword = $wlist->next ) {

    my $word = $swishword->word;
    if ( $word eq 'now' ) {
        is( $swishword->position, 14, "now position" );
    }
    if ( $word eq 'time' ) {
        is( $swishword->position, 17, "time position" );
    }
    if ( $word eq 'be' ) {
        is( $swishword->position, 25, "be position" );
    }

    #diag( '=' x 60 );
    for my $w ( SWISH_WORD_FIELDS() ) {

        #diag( sprintf( "%15s: %s\n", $w, $swishword->$w ) );

    }
}
