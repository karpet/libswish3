package SWISH::3::Config;

use 5.8.3;
use strict;
use warnings;
use Carp;

our $VERSION = '0.01';

#use Data::Dump qw(pp);
#use Devel::Peek;


use base qw( Exporter );
our @EXPORT = qw(
  SWISH_INCLUDE_FILE
  SWISH_PROP
  SWISH_PROP_MAX
  SWISH_PROP_SORT
  SWISH_META
  SWISH_MIME
  SWISH_PARSERS
  SWISH_INDEX
  SWISH_ALIAS
  SWISH_WORDS

  );

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = $class->init;
    return $self;
}

sub properties
{
    $_[0]->subconfig(SWISH_PROP());
}

sub metanames
{
    $_[0]->subconfig(SWISH_META());
}

sub mimes
{
    $_[0]->subconfig(SWISH_MIME());
}

sub parsers
{
    $_[0]->subconfig(SWISH_PARSERS());
}

1;
__END__

=head1 NAME

SWISH::3::Config - Swish3 configuration

=head1 SYNOPSIS

 use SWISH::3::Config;
 
 my $config = SWISH::3::Config->new;
                
 $config->add( I<config_file> )
 
 $config->debug;
 
=head1 DESCRIPTION

SWISH::3::Config is a Perl interface to B<libswish3> configuration functions.

=head1 SEE ALSO

L<http://swish-e.org/>

SWISH::3

=head1 AUTHOR

Peter Karman, E<lt>perl@peknet.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2007 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself. 

=cut
