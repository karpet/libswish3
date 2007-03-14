package SWISH::3::Xapian::Indexer;

use strict;
use warnings;

use Carp;
use Search::Xapian ':db';
use base qw( SWISH::3::Indexer );
use SWISH::3::Xapian::Doc;

my $doc_class   = 'SWISH::3::Xapian::Doc';
my $prefix = $SWISH::3::Xapian::Doc::term_prefix;

our $VERSION = '0.01';

$ENV{XAPIAN_PREFER_FLINT} = 1;  # force Flint db until Xapian 1.0 makes it default

__PACKAGE__->mk_accessors(qw( x_db ));

sub init
{
    my $self = shift;
    $self->SUPER::init(@_);
    $self->{x_db} =
      Search::Xapian::WritableDatabase->new($self->index->path,
              $self->index->clobber ? DB_CREATE_OR_OVERWRITE: DB_CREATE_OR_OPEN)
      or croak "can't create Xapian WritableDatabase: $!\n";

}

sub finish
{
    my $self = shift;
    $self->SUPER::finish(@_);

    # DESTROY will flush and clean up so nothing else to do
}

sub fetch
{
    my $self = shift;
    my $uri  = shift or croak "$prefix required";

    my $term = join(':', $prefix, $uri);

    if ($self->x_db->term_exists($term))
    {
        my $did  = $self->x_db->postlist_begin($term);
        my $xdoc = $self->x_db->get_document($did)
          or croak "can't get Xapian document $did";
        my $doc = $doc_class->new(uri => $uri);
        $doc->x_doc($xdoc);
        $doc->x_doc_id($did);
        return $doc;
    }
    else
    {
        return undef;
    }
}

sub insert
{
    my $self = shift;
    my $doc = shift or croak "$doc_class object required";
    $doc->isa($doc_class)
      or croak "$doc_class object required";

    $doc->x_doc_id($self->x_db->add_document($doc->x_doc));

    $self->{doc_count}++;

    if (!$self->doc_count % $self->flush_at)
    {
        eval { $self->x_db->flush };
        if ($@)
        {
            croak "bad db flush: $! $@";
        }
    }

    return $doc;

}

sub update
{
    my $self   = shift;
    my $olddoc = shift or croak "old $doc_class object required";
    my $newdoc = shift or croak "new $doc_class object required";
    $olddoc->isa($doc_class)
      or croak "old $doc_class object required";
    $newdoc->isa($doc_class)
      or croak "new $doc_class object required";

    $olddoc->x_doc_id or croak "old doc has no Xapian document id";

    $newdoc->x_doc->isa('Search::Xapian::Document')
      or croak "newdoc does not have initialized Search::Xapian::Document";

    $self->x_db->replace_document($olddoc->x_doc_id, $newdoc->x_doc);
}

sub delete
{
    my $self = shift;
    my $doc = shift or croak "$doc_class object required";
    $doc->isa($doc_class)
      or croak "$doc_class object required";

    $doc->x_doc_id or croak "document has no Xapian document id";

    $self->x_db->delete_document($doc->x_doc_id);
}

1;
__END__


=head1 NAME

SWISH::3::Xapian::Indexer - Xapian backend for Swish3

=head1 SYNOPSIS

  # TODO               
  

=head1 DESCRIPTION

SWISH::3::Xapian::Indexer is an implentation of SWISH::3::Indexer 
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
SWISH::3::Parser word tokenizer).

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

TODO compression

=head2 Mappings

The mappings for both the MetaName 2-byte strings and the PropertyNames integers 
are stored in the C<swish_header.xml> file written by finish(). See SWISH::3::Config.

=head1 SEE ALSO

SWISH::3::Indexer

http://www.xapian.org/


=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2007 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
