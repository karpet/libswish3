package SWISH::3::Index;

use strict;
use warnings;
use base qw( SWISH::3::Object );
use Path::Class;
use File::Path;
use Carp;

__PACKAGE__->mk_accessors(qw( path clobber ));

sub init
{
    my $self = shift;

    unless ($self->path)
    {
        croak "path required to create SWISH::3::Index";
    }

    if (-d $self->path)
    {

        if ($self->clobber)
        {
            rmtree([$self->path], 1, 1);
        }

    }
    elsif (-e $self->path)
    {
        croak $self->path
          . " is not a directory -- won't even attempt to clobber";
    }

    mkpath([$self->path], 1);    # should ignore if already -d

}

1;
