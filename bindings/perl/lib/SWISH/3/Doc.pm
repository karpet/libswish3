package SWISH::3::Doc;

use strict;
use warnings;
use Carp;
use Data::Dump qw/ pp /;
use Devel::Peek;

our $VERSION = '0.01';

sub handler
{
    my $self = shift;
    #warn Dumper $self;
    #Dump $_[0];
}

sub DESTROY
{
    #carp "about to DESTROY " . __PACKAGE__ . " object";
    #Dump $_[0];
}

1;
