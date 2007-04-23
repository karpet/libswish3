use strict;
use Test::More tests => 13;
#use Carp;
#use Devel::Peek;

# TODO this test reveals the [possibly very flawed] logic in our
# C-struct reference counting.
# we need a way to know when to free the struct that
# is blessed in an object. we don't want to free it with every
# DESTROY, since there might be multiple Perl objects pointing
# at the same C pointer, and freeing the underlying pointer
# will segfault any remaining Perl objects.

use_ok('SWISH::3::Parser');
use_ok('SWISH::3::Config');

ok(my $parser = SWISH::3::Parser->new(handler => sub { }), "new parser");

ok(my $conf1 = $parser->get_config, "get initial config");
ok(my $config = SWISH::3::Config->new, "new config");
ok(!$parser->set_config($config), "set config");
ok($parser->get_config->isa('SWISH::3::Config'),
    "get config isa SWISH::3::Config");
ok(my $conf2 = $parser->get_config, "get conf2");

ok(my $ana1 = $parser->get_analyzer, "get initial analyzer");
ok(my $analyzer = SWISH::3::Analyzer->new, "new analyzer");
ok(!$parser->set_analyzer($analyzer), "set analyzer");
ok($parser->get_analyzer->isa('SWISH::3::Analyzer'),
    "get analyzer isa SWISH::3::Analyzer");
ok(my $ana2 = $parser->get_analyzer, "get ana2");
