package SWISH::3::Doc;

use strict;
use warnings;
use Carp;
use base qw( SWISH::3::Object );

our $VERSION = '0.01';
use SWISH::3::Constants;

__PACKAGE__->mk_accessors(SWISH_DOC_FIELDS);

sub init
{
    my $self = shift;
    unless ($self->uri)
    {
        croak "SWISH::3::Doc object requires at least a uri";
    }
}

# TODO some utility classes for standardizing
# META names and property ids

sub add_word        { croak "must override" }
sub add_property    { croak "must override" }
sub delete_word     { croak "must override" }
sub delete_property { croak "must override" }
sub words           { croak "must override" }
sub props           { croak "must override" }

sub ok_word
{
    my $self = shift;
    unless (@_)
    {
        croak "need word value";
    }
    if (!@_ % 2)
    {
        return {@_};
    }
    else
    {
        croak "bad word format";
    }
}

sub ok_property
{
    my $self = shift;
    unless (@_)
    {
        croak "need property value";
    }
    if (@_ == 1)
    {
        return $_[0];
    }
    elsif (!@_ % 2)
    {
        return {@_};
    }
    else
    {
        croak "bad property format";
    }
}

1;
__END__


=head1 NAME

SWISH::3::Doc - Swish3 document class

=head1 SYNOPSIS

  use SWISH::3::Doc;
  use SWISH::3::Indexer;
  use SWISH::3::Constants;
  
  my $indexer = SWISH::3::Indexer->new();
  
  # create a doc object for indexing
  
  my $doc = SWISH::3::Doc->new(
                uri         => 'http://swish-e.org/docs/readme.html',
                mtime       => 1234567890,
                size        => 101,
                mime        => 'text/html',
                encoding    => 'UTF-8',
                nwords      => 2003,
                ext         => 'html'
                parser      => 'HTML'
                );
                
  for my $w ( your_function_to_parse_doc() )
  {
    my @word;
    for my $field (SWISH_WORD_FIELDS)
    {
      push(@word, $field => $w->$field);
    }
    $doc->add_word( @word );
  }
    
  for my $p ( your_function_to_get_properties() )
  {
    # as hashref
    $doc->add_property({
        id      => $p->id,
        value   => $p->value
        });
    # or as array
    $doc->add_property($p->id => $p->value);
  }
  
  $indexer->insert( $doc );
  
  # fetch a doc from index and see what it contains
  
  my $doc = $indexer->fetch( $doc->uri );
  
  my $wordlist = $doc->words;
  my $props    = $doc->props;
  
  # edit the doc and put it back in the index
  
  $doc->delete_word( $someword );
  $doc->delete_property( $someprop );
  
  $index->update( $doc->uri, $doc );


=head1 DESCRIPTION

=head1 METHODS


=head1 SEE ALSO

SWISH::3, SWISH::3::Indexer, SWISH::3::Searcher, SWISH::3::Parser


=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2007 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
