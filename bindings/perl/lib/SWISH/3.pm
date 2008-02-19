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
use constant SWISH_WORD_FIELDS =>
    qw( word position metaname context start_offset end_offset );

# load the XS at runtime, since we need $VERSION
require XSLoader;
XSLoader::load( __PACKAGE__, $VERSION );

# our symbol table is populated with newCONSTSUB in 3.xs
# directly from libswish3.h, so we just grep the symbol table.
my @constants = ( grep {m/^SWISH_/} keys %SWISH::3:: );

our %EXPORT_TAGS = ( 'constants' => [@constants], );
our @EXPORT_OK = ( @{ $EXPORT_TAGS{'constants'} } );

# convenience accessors
*config   = \&get_config;
*analyzer = \&get_analyzer;
*regex    = \&get_regex;
*parser   = \&get_parser;

sub new {
    my $class = shift;
    my %arg   = @_;
    my $self  = $class->init;

    if ( $arg{config} ) {
        $self->get_config->add( $arg{config} );
    }

    #defaults
    for my $param (qw( data_class parser_class config_class analyzer_class ))
    {
        my $method = 'set_' . $param;

        #warn "$method";
        $self->$method( $arg{$param} ) if exists $arg{param};
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

    select(STDERR);
    print '~' x 80, "\n";

    my $props = $data->config->properties;

    print "Properties\n";
    for my $p ( keys %$props ) {
        my $v    = $data->property($p);
        my $type = $props->{$p};
        print "    <$p type='$type'>$v</$p>\n";
    }

    print "Doc\n";
    for my $d (SWISH_DOC_FIELDS) {
        printf( "%15s: %s\n", $d, $data->doc->$d );
    }

    print "WordList\n";
    while ( my $swishword = $data->wordlist->next ) {
        print '-' x 50, "\n";
        for my $w (SWISH_WORD_FIELDS) {
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
