package SWISH::3::Indexer::Doc;

use strict;
use warnings;
use Carp;
use base qw( SWISH::3::Object );

our $VERSION = '0.01';
use SWISH::3;

__PACKAGE__->mk_accessors(@SWISH::3::Fields);


sub init
{
    my $self = shift;
    
    # only URI is required
    unless(defined $self->uri)
    {
        croak "URI required in new() SWISH::3::Indexer::Doc";
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

1;
__END__


=head1 NAME

SWISH::Doc - Swish-e document class

=head1 SYNOPSIS

  use SWISH::Doc;
  use SWISH::Index;
  
  my $index = SWISH::Index->new();
  
  # create a doc object for indexing
  
  my $doc = SWISH::Doc->new(
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
    $doc->add_word( 
        metaname        => $w->metaname
        word            => $w->word,
        start_offset    => $w->start_offset,
        end_offset      => $w->end_offset,
        position        => $w->position
        );
  }
    
  for my $p ( your_function_to_get_properties() )
  {
    $doc->add_property(
        id      => $p->id,
        value   => $p->value
        );
  }
  
  $index->add_doc( $doc );
  
  # fetch a doc from index and see what it contains
  
  my $doc = $index->get_doc( $doc->uri );
  
  my $wordlist = $doc->words;
  my $props    = $doc->props;
  
  # edit the doc and put it back in the index
  
  $doc->delete_word( $someword );
  $doc->delete_property( $someprop );
  
  $index->replace_doc( $doc->uri, $doc );


=head1 DESCRIPTION

=head1 METHODS


=head1 SEE ALSO

SWISH::Index, SWISH::Parser, SWISH::Config, SWISH::Search


=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
