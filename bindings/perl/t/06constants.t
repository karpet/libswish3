use Test::More tests => 3;

use SWISH::3::Constants;

is( SWISH_MIME,                'MIME',          SWISH_MIME );
is( SWISH_PROP,                'PropertyNames', SWISH_PROP );
is( scalar(SWISH_WORD_FIELDS), 6,               'SWISH_WORD_FIELDS' );
