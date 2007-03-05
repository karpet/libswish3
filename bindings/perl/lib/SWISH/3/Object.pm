package SWISH::3::Object;

use 5.8.3;
use strict;
use warnings;
use base qw( Class::Accessor::Fast );
use Carp;
use Data::Dump qw(dump);

our $VERSION = '0.01';

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless($self, $class);
    $self->_init(@_);
    $self->init(@_);
    return $self;
}

sub _init
{
    my $self = shift;
    if (@_)
    {
        my %extra = @_;
        @$self{keys %extra} = values %extra;
    }
}

1;
