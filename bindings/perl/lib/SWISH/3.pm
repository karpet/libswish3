use strict;
use warnings;
use 5.008_003;

package SWISH::3;

our $VERSION = '0.01';

# set by libswish3 in swish.c but that happens after %ENV has been
# initialized at Perl compile time.
$ENV{SWISH3} = 1;

use Carp;
use Data::Dump;
use Devel::Peek;

use base qw( Exporter );

use constant SWISH_DOC_FIELDS =>
    qw( mtime size encoding mime uri nwords ext parser );
use constant SWISH_TOKEN_FIELDS =>
    qw( pos meta value context len );

# load the XS at runtime, since we need $VERSION
require XSLoader;
XSLoader::load( __PACKAGE__, $VERSION );

# init the memory counter as class method at start up
# and call debug in END block
SWISH::3->init;

END {
    my $memcount = SWISH::3->get_memcount;
    if ($memcount) {
        warn "suspicious memory count in global cleanup";
        SWISH::3->mem_debug();
    }
}

# our symbol table is populated with newCONSTSUB in Constants.xs
# directly from libswish3.h, so we just grep the symbol table.
my @constants = ( grep {m/^SWISH_/} keys %SWISH::3:: );

our %EXPORT_TAGS = ( 'constants' => [@constants], );
our @EXPORT_OK = ( @{ $EXPORT_TAGS{'constants'} } );

# convenience accessors
*config   = \&get_config;
*analyzer = \&get_analyzer;
*regex    = \&get_regex;
*parser   = \&get_parser;

# alias debugging methods for all classes
*SWISH::3::Config::refcount   = \&refcount;
*SWISH::3::Analyzer::refcount = \&refcount;
*SWISH::3::WordList::refcount = \&refcount;
*SWISH::3::Word::refcount     = \&refcount;
*SWISH::3::Doc::refcount      = \&refcount;
*SWISH::3::Data::refcount     = \&refcount;

sub new {
    my $class = shift;
    my %arg   = @_;
    my $self  = $class->_setup;

    if ( $arg{config} ) {
        $self->get_config->add( $arg{config} );
    }

    # override defaults
    for my $param (qw( data_class parser_class config_class analyzer_class ))
    {
        my $method = 'set_' . $param;

        if ( exists $arg{$param} ) {

            #warn "$method";
            $self->$method( $arg{$param} );
        }
    }

    $arg{handler} ||= \&default_handler;

    $self->set_handler( $arg{handler} );

    # KS default regex -- should also match swish_tokenize() behaviour
    $arg{regex} ||= qr/\w+(?:'\w+)*/;
    $self->set_regex( $arg{regex} );

    return $self;
}

sub parse {
    my $self = shift;
    my $what = shift
        or croak "parse requires filehandle, scalar ref or file path";
    if ( ref $what eq 'SCALAR' ) {
        return $self->parse_buffer($what);
    }
    elsif ( ref $what ) {
        return $self->parse_fh($what);
    }
    else {
        return $self->parse_file($what);
    }
}

sub dump {
    my $self = shift;
    if (@_) {
        Dump(@_);
        Data::Dump::dump(@_);
    }
    else {
        Dump($self);
        Data::Dump::dump($self);
    }
}

sub default_handler {
    my $data = shift;
    unless ( $ENV{SWISH_DEBUG} ) {
        warn "default handler called\n";
        return;
    }

    select(STDERR);
    print '~' x 80, "\n";

    my $props     = $data->properties;
    my $prop_hash = $data->config->get_properties;

    print "Properties\n";
    for my $p ( sort keys %$props ) {
        print " key: $p\n";
        my $prop_value = $props->{$p};
        print " value: " . Data::Dump::dump($prop_value) . "\n";
        my $prop = $prop_hash->get($p);
        printf( "    <%s type='%s'>%s</%s>\n",
            $prop->name, $prop->type, $data->property($p), $prop->name );
    }

    print "Doc\n";
    for my $d (SWISH_DOC_FIELDS) {
        printf( "%15s: %s\n", $d, $data->doc->$d );
    }

    print "WordList\n";
    my $wordlist = $data->wordlist;
    while ( my $swishword = $wordlist->next ) {
        print '-' x 50, "\n";
        for my $w (SWISH_TOKEN_FIELDS) {
            printf( "%15s: %s\n", $w, $swishword->$w );
        }
    }
}

1;
__END__

=head1 NAME

SWISH::3 - Perl interface to libswish3

=head1 SYNOPSIS

 use SWISH::3;
 my $swish3 = SWISH::3->new(
                config      => 'path/to/config.xml',
                handler     => \&my_handler,
                regex       => qr/\w+(?:'\w+)*/,
                );
 $swish3->parse( 'path/to/file.xml' )
    or die "failed to parse file: " . $swish3->error;
 
 printf "libxml2 version %s\n", $swish3->xml2_version;
 printf "libswish3 version %s\n", $swish3->version;
 
 
=head1 DESCRIPTION

SWISH::3 is a Perl interface to the libswish3 C library.


=head1 METHODS

=head2 xml2_version

Returns the libxml2 version used by libswish3.

=head2 version

Returns the libswish3 version.


=head1 SEE ALSO

L<http://swish-e.org/>

SWISH::Prog

=cut
