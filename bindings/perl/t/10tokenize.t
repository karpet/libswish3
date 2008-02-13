use Test::More tests => 9;

use_ok('SWISH::3');
use_ok('SWISH::3::Constants');

ok( my $s3 = SWISH::3->new, "new s3" );
ok( my $analyzer = $s3->analyzer, "new tokenizer" );

ok( my $wlist = $analyzer->tokenize(
        "now is the time, ain't it? or when else might it be!",
        14, 5, 'foo', 'bar'
    ),
    "wordlist"
);

ok( $wlist->isa('SWISH::3::WordList'), 'isa wordlist' );

while ( my $swishword = $wlist->next ) {

    my $word = $swishword->word;
    if ( $word eq 'now' ) {
        is( $swishword->position, 6, "now position" );
    }
    if ( $word eq 'time' ) {
        is( $swishword->position, 9, "time position" );
    }
    if ( $word eq 'be' ) {
        is( $swishword->position, 17, "be position" );
    }

    #diag( '=' x 60 );
    for my $w ( SWISH_WORD_FIELDS() ) {

        #diag( sprintf( "%15s: %s\n", $w, $swishword->$w ) );

    }
}
