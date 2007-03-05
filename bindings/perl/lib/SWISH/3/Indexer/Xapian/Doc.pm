package SWISH::3::Indexer::Xapian::Doc;

use strict;
use warnings;
use Carp;
use base qw( SWISH::3::Indexer::Doc );
use Search::Xapian::Document;

__PACKAGE__->mk_accessors(qw( x_doc x_doc_id ));

our $VERSION = '0.01';

# TODO
# trick is mapping between the basic SWISH::Doc structure
# and the Xapian::Document structure
# not much to it really: just save properties as 'value'
# and words as 'posting'
# swishdescription could be saved as 'data'
# uri gets saved as 'term' (TODO why? does xapian use that as for unique id?)



1;
__END__


=head1 NAME

SWISH::3::Indexer::Xapian::Doc

=head1 SYNOPSIS

  use SWISH::3::Indexer::Xapian::Doc;
  
  # see SWISH::Doc documentation


=head1 DESCRIPTION

=head1 METHODS


=head1 SEE ALSO

SWISH::3::Indexer::Doc

=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
