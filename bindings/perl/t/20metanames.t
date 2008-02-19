use Test::More tests => 22;
use Data::Dump qw( dump );

use_ok('SWISH::3');

ok( my $s3 = SWISH::3->new(
        config  => '<swishconfig><MetaNames>foo</MetaNames></swishconfig>',
        handler => \&getmeta
    ),
    "new parser"
);

my $r = 0;
while ( $r < 10 ) {
    ok( $r += $s3->parse_file("t/test.html"), "parse HTML" );

    #diag("r = $r");
}
$r = 0;
while ( $r < 10 ) {
    ok( $r += $s3->parse_file("t/test.xml"), "parse XML" );

    #diag("r = $r");
}

sub getmeta {
    my $data = shift;

    #diag(dump($data->metanames));

    #$data->wordlist->debug;


}
