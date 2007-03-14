package SWISH::3::Xapian::Doc;

use strict;
use warnings;
use Carp;
use base qw( SWISH::3::Doc );
use Search::Xapian::Document;

__PACKAGE__->mk_accessors(qw( x_doc x_doc_id ));

our $VERSION = '0.01';

our $term_prefix = 'URI';

sub init
{
    my $self = shift;
    $self->SUPER::init(@_);
    my $xd = Search::Xapian::Document->new
      or croak "can't create Search::Xapian::Document object";
    $self->x_doc($xd);
    $self->x_doc->add_term(join(':', $term_prefix, $self->uri));
    return $self;
}

# TODO
# trick is mapping between the basic SWISH::3::Doc structure
# and the Xapian::Document structure
# not much to it really: just save properties as 'value'
# and words as 'posting'
# swishdescription could be saved as 'data'
# uri gets saved as 'term' (TODO why? does xapian use that as for unique id?)

# set_data() can be used to store whatever you want
# but can't be sorted on. so it's like an unsortable property

# add_value() is similar to Swish-e properties
# results can be sorted by 'value'
# and each 'value' needs a unique number (like a property id)

# add_term() is where you would add a word with no positional info
# could have META: prefixed however and optional 'weight' as second param
# we use uri as unique term

# add_posting() is for adding specific words with positional info
# can take 'weight' as optional 3rd param
# we add twice so a word can be found 'naked' and with explicit metaname prefix

sub add_word
{
    my $self = shift;
    my $word = $self->SUPER::ok_word(@_);

    $word->x_doc->add_posting(
                              join(':',
                                   $self->meta($word->{metaname}),
                                   $word->{word}),
                              $word->{position},
                              $self->meta_bias($word->{metaname}) || 1
                             );

    # TODO make this configurable
    $word->x_doc->add_posting($word->{word}, $word->{position},
                              $self->meta_bias($word->{metaname}) || 1);

}

sub add_property
{
    my $self = shift;
    my $prop = $self->SUPER::ok_property(@_);

}

sub delete_word
{

}

sub delete_property
{

}

sub words
{

}

sub props
{

}

sub meta
{
    my $self = shift;
    my $mn   = shift;

    # return abbreviation

}

sub meta_bias
{
    my $self = shift;
    my $mn   = shift;

    # return bias - look up from config? memoize

}

1;
__END__


=head1 NAME

SWISH::3::Xapian::Doc

=head1 SYNOPSIS

  use SWISH::3::Xapian::Doc;
  
  # see SWISH::Doc documentation


=head1 DESCRIPTION

=head1 METHODS


=head1 SEE ALSO

SWISH::3::Doc

=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2007 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut
