package SWISH::3::MetaName;

use strict;
use warnings;
use Carp;
use Data::Dump qw/ pp /;

use base qw( Class::Accessor::Fast );
__PACKAGE__->mk_accessors(qw/ name bias /);

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless($self, $class);
    $self->_init(@_);
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

__END__


