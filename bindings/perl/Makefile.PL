use ExtUtils::MakeMaker;

WriteMakefile(
    NAME         => 'SWISH::3',
    VERSION_FROM => 'lib/SWISH/3.pm',
    PREREQ_PM    => {
        'Test::More'  => 0,
        'Devel::Peek' => 0,
        'Data::Dump'  => 0,
    },
    H => [qw( xs_helpers.c xs_boiler.h headers.h macros.h )],
    (   $] >= 5.005
        ? ( ABSTRACT_FROM => 'lib/SWISH/3.pm',
            AUTHOR        => 'Peter Karman <perl@peknet.com>'
            )
        : ()
    ),
    LIBS => ['-L/usr/lib -L/usr/local/lib -lswish3 -lxml2'],
    DEFINE => '',    # e.g., '-DHAVE_SOMETHING'
    INC =>
        '-I/usr/local/include -I/usr/local/include/libxml2 -I/usr/include -I/usr/include/libxml2 -I. -I../../src/libswish3',

    clean => {
        FILES =>
            't/swish.xml'
    },

    # Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too
);

sub MY::postamble {

    build_constants();

    return
          "\$(XS_FILES): "
        . join( " ", <XS/*.xs> )
        . "\n\ttouch \$(XS_FILES)";
}

sub build_constants {
    my @defines = `grep define ../../src/libswish3/libswish3.h | grep SWISH_`;
    my @constants;
    for my $def (@defines) {
        chomp $def;
        if ( $def =~ m/^#define (\w+)\ +(\d+)/ ) {
            my $const = $1;
            push( @constants,
                qq{newCONSTSUB(stash, "$const", newSViv($const));} );
        }
        elsif ( $def =~ m/^#define (\w+)\ +"(.+?)"/ ) {
            my $const = $1;
            push( @constants,
                qq{newCONSTSUB(stash, "$const", newSVpv($const, 0));} );
        }
    }
    open( XS, "> XS/Constants.xs" ) or die "can't write XS/Constants.xs: $!";
    print XS '
BOOT:
{
    HV *stash;
  
    stash = gv_stashpv("SWISH::3", TRUE);
';

    for my $const (@constants) {
        print XS "    $const\n";
    }
    print XS "}\n";
    close(XS);

}
