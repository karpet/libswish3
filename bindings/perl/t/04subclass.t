package MyApp;

use Test::More tests => 201;

use base qw( SWISH::3::Parser );

ok(
    my $parser = MyApp->new(
        config  => 't/t.conf',
        handler => sub {         
            #print 'foo';  # print() causes err under Test, warn() doesn't...
            #warn 'foo';
        }
    ),
    "new object with config"
  );

#$parser->debug;

my $loops = 0;
while ($loops++ < 100)
{
    ok($r = $parser->parse_file('t/test.html'), "parse HTML filesystem");
    ok($r = $parser->parse_file('t/test.xml'),  "parse XML filesystem");
}

1;


package MyApp::Data;
use base qw( SWISH::3::Parser::Data );
1;


package MyApp::Doc;
use base qw( SWISH::3::Parser::Doc );
1;

package MyApp::Property;
use base qw( SWISH::3::Parser::Property );
1;

package MyApp::Word;
use base qw( SWISH::3::Parser::Word );
1;

package MyApp::WordList;
use base qw( SWISH::3::Parser::WordList );
1;
