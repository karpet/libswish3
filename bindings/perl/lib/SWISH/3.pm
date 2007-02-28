package SWISH::3;

use strict;
use warnings;

our $VERSION = '0.01';

use Carp;
use Data::Dump qw(pp);

use base qw(Class::Accessor::Fast);

require XSLoader;
XSLoader::load('SWISH::3', $VERSION);

# defer till runtime so any constants can load
require SWISH::3::Config;
require SWISH::3::Data;
require SWISH::3::Doc;
require SWISH::3::MetaName;
require SWISH::3::Property;
require SWISH::3::Word;
require SWISH::3::WordList;
require SWISH::Doc;

use Devel::Peek;


__PACKAGE__->mk_accessors(qw/ config handler /);

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};

    bless($self, $class);
    $self->_make_subclasses;
    $self->_init(@_);
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

sub _init
{
    my $self = shift;
    if (@_)
    {
        my %extra = @_;
        @$self{keys %extra} = values %extra;
    }

    my $conf = SWISH::3::Config->new;

    if ($self->config)
    {
        $conf->add($self->config);
    }

    $self->config($conf);

    unless (exists($self->{handler}) && ref($self->{handler}) eq 'CODE')
    {
        carp
          "WARNING: using default 3 handler -- that's likely not what you want";
        $self->handler(\&def_handler);
    }
}

sub debug
{
    my $self = shift;
    
    $self->config->debug;

}

sub def_handler
{
    my $data = shift;

    select(STDERR);

    #print '~' x 80, "\n";

    my $props = $data->config->properties;

    #print "Properties\n";
    for my $p (keys %$props)
    {
        my $v    = $data->property($p);
        my $type = $props->{$p};

        #print "    <$p type='$type'>$v</$p>\n";
    }

    #print "Doc\n";
    for my $d (@SWISH::Doc::Fields)
    {

        #printf("%15s: %s\n", $d, $data->doc->$d);
    }

    #print "WordList\n";
    while (my $swishword = $data->wordlist->next)
    {
        for my $w (@SWISH::3::Word::Fields)
        {

            #printf("%15s: %s\n", $w, $swishword->$w);
        }
    }

}

1;
__END__

=head1 NAME

SWISH::3 - interface to libswish3

=head1 SYNOPSIS

 use SWISH::3;
 
 my $parser = SWISH::3->new( 
                config  => 'path/to/my_file.conf',
                handler => \&handler
                );
                
 my $indexer = MyIndexer->new();
 
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

=head2 libxml2_version

=head2 slurp_file


=head1 SEE ALSO

L<http://swish-e.org/>

SWISH::Prog, SWISH::Index, SWISH::Doc, SWISH::Search

=cut
