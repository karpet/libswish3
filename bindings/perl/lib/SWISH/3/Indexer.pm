package SWISH::3::Indexer;

use strict;
use warnings;
use base qw( SWISH::3::Object );
use Carp;
use SWISH::3::Index;

our $VERSION = '0.01';
__PACKAGE__->mk_accessors(qw( index path flush_at clobber config doc_count ));

sub init
{
    my $self = shift;
    $self->{_start} = time();
    $self->{flush_at} ||= 10000;
    $self->{index}    ||=
      SWISH::3::Index->new(path    => $self->path,
                           clobber => $self->clobber || 0);

}

sub finish
{
    my $self = shift;
    unless ($self->config)
    {
        carp "no config() set in Indexer - swish_header.xml cannot be written";
        return;
    }

}

sub fetch  { croak "must override fetch" }
sub insert { croak "must override insert" }
sub update { croak "must override update" }
sub delete { croak "must override delete" }

1;
__END__


=head1 NAME

SWISH::3::Indexer - build a Swish3 index

=head1 SYNOPSIS

  use SWISH::3::Indexer;
  use SWISH::3::Config;
  
  my $config = SWISH::3::Config->new();
  
  my $indexer = SWISH::3::Indexer->new(
                path        => 'index.swish-e',
                flush_at    => 10000,
                clobber     => 1,
                config      => $config
                );
                
  for my $doc (@bunch_of_doc_objects)
  {    
    if (my $old_doc = $indexer->fetch($doc->uri))
    {
        $indexer->update($old_doc, $doc);
    }
    else
    {
        $indexer->insert($doc);
    }
    
    # oops! change your mind?
    $indexer->delete( $doc );
  }
  
  $indexer->finish;
  
  print $indexer->doc_count . " documents indexed";
                
  

=head1 DESCRIPTION

SWISH::3::Indexer is a basic API class intended for subclassing for specific
IR library implementations.

=head1 METHODS

=head2 new


=head2 init

Called by new(). Intended to be overridden instead of new().

=head2 doc_exists( I<doc> )

=head2 add_doc( I<doc> )

=head2 replace_doc( I<doc> )

=head2 delete_doc( I<doc> )

=head2 finish

=head1 SEE ALSO

SWISH::3::Parser

=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
