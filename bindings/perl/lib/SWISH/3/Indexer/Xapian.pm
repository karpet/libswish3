package SWISH::3::Indexer::Xapian;

use strict;
use warnings;

use Carp;
use Search::Xapian ':db';
use base qw( SWISH::3::Indexer );

our $VERSION = '0.01';

__PACKAGE__->mk_accessors(qw( db ));

sub init
{
    my $self = shift;
    $self->{db} =
      Search::Xapian::WritableDatabase->new($self->name,
                   $self->overwrite ? DB_CREATE_OR_OVERWRITE: DB_CREATE_OR_OPEN)
      or croak "can't create Xapian WritableDatabase: $!\n";

}

sub finish
{

    # DESTROY will flush and clean up so nothing to do
}

sub get_doc
{
    my $self = shift;
    my $uri  = shift or croak "URI required";

    if ($self->db->term_exists('URI' . $uri))
    {
        my $did  = $self->db->postlist_begin('URI' . $uri);
        my $xdoc = $self->db->get_document($did)
          or croak "can't get Xapian document $did";
        my $doc = SWISH::3::Indexer::Xapian::Doc->new(uri => $uri);
        $doc->x_doc($xdoc);
        $doc->x_doc_id($did);
        return $doc;
    }
    else
    {
        return undef;
    }
}

sub add_doc
{
    my $self = shift;
    my $doc = shift or croak "SWISH::Doc::Xapian object required";
    $doc->isa('SWISH::Doc::Xapian')
      or croak "SWISH::Doc::Xapian object required";

    $doc->x_doc_id($self->db->add_document($doc->x_doc));

    $self->{doc_count}++;

    if (!$self->doc_count % $self->flush_at)
    {
        eval { $self->db->flush };
        if ($@)
        {
            croak "bad db flush: $! $@";
        }
    }

    return $doc;

}

sub replace_doc
{
    my $self   = shift;
    my $olddoc = shift or croak "old SWISH::Doc::Xapian object required";
    my $newdoc = shift or croak "new SWISH::Doc::Xapian object required";
    $olddoc->isa('SWISH::Doc::Xapian')
      or croak "old SWISH::Doc::Xapian object required";
    $newdoc->isa('SWISH::Doc::Xapian')
      or croak "new SWISH::Doc::Xapian object required";

    $olddoc->x_doc_id or croak "old doc has no Xapian document id";

    $newdoc->x_doc->isa('Search::Xapian::Document')
      or croak "newdoc does not have initialized Search::Xapian::Document";

    $self->db->replace_document($olddoc->x_doc_id, $newdoc->x_doc);
}

sub delete_doc
{
    my $self = shift;
    my $doc = shift or croak "SWISH::Doc::Xapian object required";
    $doc->isa('SWISH::Doc::Xapian')
      or croak "SWISH::Doc::Xapian object required";

    $doc->x_doc_id or croak "document has no Xapian document id";

    $self->db->delete_document($doc->x_doc_id);
}

1;
__END__


=head1 NAME

SWISH::3::Indexer::Xapian - Xapian backend for Swish3

=head1 SYNOPSIS

  # TODO               
  

=head1 DESCRIPTION

SWISH::3::Indexer::Xapian is an implentation of SWISH::3::Indexer 
for Xapian (http://www.xapian.org/).

=head1 METHODS

Only methods that extend SWISH::3::Indexer are documented here.


=head1 METANAMES and PROPERTYNAMES

Swish-e's MetaNames and PropertyNames features are implemented with Xapian in the following
ways:

=head2 MetaNames

Each word in a document is added to a Xapian index using add_posting(). The MetaNames
associated with each word are added as unique prefixes to the word string, separated
from the word by a colon. The colon separator is to reduce possible confusion
if words are not lowercased (though lowercasing is the default behaviour of the 
SWISH::Parser word tokenizer).

For example, words that are associated with the C<swishtitle> MetaName might be stored as:

 XM:foo
 
where C<XM> is a guaranteed unique 2-byte string. Every MetaName gets its own 2-byte
string.

Then at search time, the 2-byte strings are mapped to the original MetaName value
with Xapian's QueryParser's add_prefix() method.

This method of 3-byte storage (2 bytes for the alpha characters and 1 byte for the colon)
compromises between a reasonable number 
of possible unique MetaNames (626), the Xapian limitation that terms must be strings,
and saves space in the index by not storing the entire metaname.

=head2 PropertyNames

Properties are stored in the Xapian index using the add_value() method. Each property
gets a unique integer.

=head2 Mappings

The mappings for both the MetaName 2-byte strings and the PropertyNames integers 
are stored in the C<swishheader.xml> file written by SWISH::Config.

=head1 SEE ALSO

SWISH::3::Indexer

http://www.xapian.org/


=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
