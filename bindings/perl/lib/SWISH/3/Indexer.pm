package SWISH::3::Indexer;

use strict;
use warnings;
use base qw( SWISH::3::Object );

our $VERSION = '0.01';

sub init
{
    my $self = shift;
    my %e    = @_;
    $self->{$_} = $e{$_} for keys %e;
    $self->{_start} = time();
    $self->mk_accessors(qw( name flush_at overwrite config doc_count ));
    $self->{flush_at} ||= 10000;
}

sub finish      { croak "must override" }
sub get_doc     { croak "must override" }
sub add_doc     { croak "must override" }
sub replace_doc { croak "must override" }
sub delete_doc  { croak "must override" }

1;
__END__


=head1 NAME

SWISH::3::Indexer - build a Swish3 index

=head1 SYNOPSIS

  use SWISH::3::Indexer;
  use SWISH::3::Config;
  
  my $config = SWISH::3::Config->new();
  
  my $index = SWISH::3::Indexer->new(
                name        => 'index.swish-e',
                flush_at    => 10000,
                overwrite   => 1,
                config      => $config
                );
                
  for my $doc (@bunch_of_doc_objects)
  {    
    if (my $old_doc = $index->get_doc($doc->uri))
    {
        $index->replace_doc($old_doc, $doc);
    }
    else
    {
        $index->add_doc($doc);
    }
    
    # oops! change your mind?
    $index->delete_doc( $doc );
  }
  
  $index->close;
  
  print $index->doc_count . " documents indexed";
                
  

=head1 DESCRIPTION

SWISH::3::Indexer is a basic API class intended for subclassing for specific
IR library implementations.

=head1 METHODS

=head2 new


Alias: open().

=head2 init

Called by new(). Intended to be overridden instead of new().

=head2 doc_exists( I<doc> )

=head2 add_doc( I<doc> )

=head2 replace_doc( I<doc> )

=head2 delete_doc( I<doc> )


=head1 SEE ALSO

SWISH::3::Parser

=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
