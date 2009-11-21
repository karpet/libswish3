use strict;
use ExtUtils::MakeMaker;
use 5.008003;
use Config;        # for path separator
use File::Spec;    # for catpath

my $MM_Version = $ExtUtils::MakeMaker::VERSION;

if ( $MM_Version =~ /_/ )    # dev version
{
    $MM_Version = eval $MM_Version;
    die $@ if ($@);
}

# find libswish3-config script.
# these routines cribbed from Swish-e 2.x Makefile.PL

#----------------------------------------------------------------------------------
# Find a program in either $PATH or path/directory passed in.
#----------------------------------------------------------------------------------
sub find_program {
    my ($name) = @_;

    my $search_path ||= $ENV{LIBSWISH3_PATH} || $ENV{PATH};

    for my $dir ( split /$Config{path_sep}/, $search_path ) {
        my $path = File::Spec->catfile( $dir, $name );

        for my $extension ( '', '.exe' ) {
            my $file = $path . $extension;
            return $file if -x $file && !-d _;
        }
    }
    return;
}

#----------------------------------------------------------------------------------
# Run a program with backtics, checking for errors
#----------------------------------------------------------------------------------

sub backtick {
    my ($command) = @_;

    my $output = `$command`;

    my $status
        = $? == 0  ? ''
        : $? == -1 ? "Failed to execute: $!"
        : $? & 127 ? sprintf(
        "Child died with signal %d, %s corefile",
        ( $? & 127 ),
        ( $? & 128 ) ? 'with' : 'without'
        )
        : sprintf( "Child exited with value %d", $? >> 8 );

    die "Failed to run program [$command]: $status\n" if $status;

    chomp $output;
    return $output;
}

###########################################
# get the config scripts and write makefile

my $libswish3_config = find_program('libswish3-config') || '';
my $xml2_config      = find_program('xml2-config')      || '';
warn "using libswish3-config $libswish3_config\n";
warn "using xml2-config $xml2_config\n";
if ( !$libswish3_config || !$xml2_config ) {

    print STDERR <<EOF;

Missing one or more dependencies.

You must have libswish3 and libxml2 installed in order to build
SWISH::3. If you have them installed, set the LIBSWISH3_PATH 
environment variable or make sure 'libswish3-config' 
and 'xml2-config' are in your PATH.

EOF

    exit(0);    # per Search::Xapian Makefile.PL, MakeMaker expects exit 0
                # to indicate failed dependencies

}

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
    LIBS => join( ' ',
        backtick( $libswish3_config . ' --libs' ),
        backtick( $xml2_config . ' --libs' ) ),
    DEFINE => '',          # e.g., '-DHAVE_SOMETHING'
    INC    => join( ' ',
        backtick( $libswish3_config . ' --cflags' ),
        backtick( $xml2_config . ' --cflags' ) ),

    clean => { FILES => 't/swish.xml' },

    # Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too

    ( $MM_Version >= 6.48 ? ( MIN_PERL_VERSION => '5.8.3' ) : () ),
    ( $MM_Version >= 6.31 ? ( LICENSE          => 'perl' )  : () ),
    (   $MM_Version <= 6.44 ? ()
        :

            (
            META_MERGE => {
                resources => {
                    license  => 'http://dev.perl.org/licenses/',
                    homepage => 'http://dev.swish-e.org/wiki/swish3',
                    bugtracker =>
                        'http://rt.cpan.org/NoAuth/Bugs.html?Dist=SWISH-3',
                    repository => 'http://svn.swish-e.org/libswish3',
                },
            }
            )
    ),
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
