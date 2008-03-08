use Test::More tests => 48;

use SWISH::3;

ok( my $s3 = SWISH::3->new(
        config => 't/t.conf'

    ),
    "new s3 object"
);

ok( my $config = $s3->config, "get config" );

#$config->debug;
#undef $config;
eval { $s3->set_config(undef) };
ok( $@, "set config with undef" );

eval { $s3->set_config( bless( {}, 'not_a_config' ) ) };
ok( $@, "set config with non-Config class" );

ok( my $properties = $s3->config->get_properties, "get properties" );

for my $name ( sort @{ $properties->keys } ) {

    #diag($name);

    my $prop = $properties->get($name);

    #diag( "$name refcount = " . $prop->refcount );

    is( $prop->id, 0, "prop id" );
    is( $name, $prop->name, "prop name" );
}

ok( my $metanames = $s3->config->get_metanames, "get metanames" );

for my $name ( sort @{ $metanames->keys } ) {

    my $meta = $metanames->get($name);

    is( $meta->id, 0, "meta id" );
    is( $name, $meta->name, "meta name" );

}

ok( my $index = $s3->config->get_index, "get index" );

my %indexv = (
    Format => 'swish',
    Locale => 'en_US.UTF-8',
    Name   => 'index.swish3'
);

for my $key ( sort keys %indexv ) {
    is( $index->get($key), $indexv{$key}, "index $key" );
}
