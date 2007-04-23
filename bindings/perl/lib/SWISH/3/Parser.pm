package SWISH::3::Parser;

use strict;
use warnings;
use Carp;
use SWISH::3;    # in case this class gets 'use'd directly
use SWISH::3::Parser::Doc;
use SWISH::3::Parser::MetaName;
use SWISH::3::Parser::Property;
use SWISH::3::Parser::Word;
use SWISH::3::Parser::WordList;
use SWISH::3::Config;
use SWISH::3::Analyzer;

use Devel::Peek;
use Data::Dump qw( pp );

our $VERSION = '0.01';

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    $class->_init_swish;
    my %args = @_;
    if (   $args{config}
        && ref($args{config})
        && $args{config}->isa('SWISH::3::Config'))
    {

        # do nothing
    }
    elsif ($args{config})
    {
        my $c = SWISH::3::Config->new;
        $c->add($args{config});
        $args{config} = $c;
    }
    else
    {
        $args{config} = SWISH::3::Config->new;
    }
    $args{analyzer} ||= SWISH::3::Analyzer->new(config => $args{config});
    unless ($args{handler})
    {
        carp(  "WARNING: using default SWISH::3::Parser::Data handler -- "
             . "that's likely not what you want");
        $args{handler} = \&SWISH::3::Parser::Doc::handler;
    }
    my $self = $class->_init($args{config}, $args{analyzer}, $args{handler});
    return $self;
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
