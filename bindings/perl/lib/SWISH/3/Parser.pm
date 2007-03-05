package SWISH::3::Parser;

use strict;
use warnings;
use base qw( SWISH::3::Object );
use SWISH::3;    # in case this class gets 'use'd directly

# require so that any constants can load
require SWISH::3::Parser::Doc;
require SWISH::3::Parser::MetaName;
require SWISH::3::Parser::Property;
require SWISH::3::Parser::Word;
require SWISH::3::Parser::WordList;

use Devel::Peek;

our $VERSION = '0.01';

__PACKAGE__->mk_accessors(qw( config handler ));

sub init
{
    my $self = shift;
    $self->_make_subclasses;
    $self->_init_config;
    $self->_init_handler;
    $self->_init_parser;
    return $self;
}

sub DESTROY
{
    my $self = shift;

    #carp "about to DESTROY 3 object";
    #Dump $self;

    $self->_free;
    $self->_cleanup;
}

sub _init_config
{
    my $self = shift;
    my $conf = SWISH::3::Config->new;

    if ($self->config)
    {
        $conf->add($self->config);
    }

    $self->config($conf);
}

sub _init_handler
{
    my $self = shift;

    unless (exists($self->{handler}) && ref($self->{handler}) eq 'CODE')
    {
        Carp::carp "WARNING: using default SWISH::3::Parser::Data handler -- "
          . "that's likely not what you want";
        $self->handler(\&SWISH::3::Parser::Doc::handler);
    }
}

sub debug
{
    my $self = shift;

    $self->config->debug;

}

1;
__END__

=head1 NAME

SWISH::3::Parser - interface to libswish3 parsers

=head1 SYNOPSIS

 use SWISH::3;
 
 my $parser = SWISH::3::Parser->new( 
                config  => 'path/to/my_file.conf',
                handler => \&handler
                );
                
 my $indexer = SWISH::3::Index->new();
 
 $indexer->open_index;
 
 for my $file (@list_of_files_from_somewhere)
 {
    $parser->parse_file($file);
 }
 
 $indexer->close_index;
 
 sub handler
 {
    my $data = shift;
    my $doc_id = $indexer->add_doc( $data );
    $indexer->add_properties( $data, $doc_id );
    $indexer->add_words( $data, $doc_id );
 }

 
=head1 DESCRIPTION



=head1 METHODS

=head2 new

=head2 parse_file

=head2 parse_buf



=head1 UTILITY METHODS

=head2 slurp_file


=head1 SEE ALSO

L<http://swish-e.org/>

SWISH::Prog, SWISH::3::Index, SWISH::3::Index::Doc, SWISH::3::Search

=cut
