package SWISH::3;

use 5.8.3;
use strict;
use warnings;

our $VERSION = '0.01';

$ENV{SWISH3} = 1;   # should be set by libswish3 in swish.c but doesn't seem to.

use Carp;
require XSLoader;
XSLoader::load('SWISH::3', $VERSION);

1;
__END__

=head1 NAME

SWISH::3 - interface to libswish3

=head1 SYNOPSIS

 use SWISH::3;
 
 my $xml2_version = SWISH::3->xml2_version;
 my $swish3_version = $SWISH::3::VERSION;
 
 
=head1 DESCRIPTION



=head1 CLASS METHODS

=head2 make_subclasses



=head1 UTILITY METHODS

=head2 xml2_version


=head1 SEE ALSO

L<http://swish-e.org/>

SWISH::Prog

=cut
