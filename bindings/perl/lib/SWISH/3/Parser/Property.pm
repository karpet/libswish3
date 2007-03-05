package SWISH::3::Parser::Property;

use strict;
use warnings;
use Carp;
use Data::Dump qw/ pp /;
use Devel::Peek;

our $VERSION = '0.01';

sub handler
{
    my $self = shift;
    my $name = shift;
    my $value = shift;
    #Dump $self;
    #warn Dumper $self;
    
    print " name = $name\n";
    print "value = $value\n";
}

1;
