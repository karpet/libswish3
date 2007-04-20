package SWISH::3::Analyzer;

use 5.8.3;
use strict;
use warnings;
use Carp;
use SWISH::3;    # in case this class gets 'use'd directly
use SWISH::3::Config;

our $VERSION = '0.01';

#use Data::Dump qw(pp);
#use Devel::Peek;

# KS default regex -- should also match swish_tokenize() behaviour
our $regex = qr/\w+(?:'\w+)*/;

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my %arg   = @_;
    $arg{config} ||= SWISH::3::Config->new;
    $arg{regex}  ||= $regex;
    my $self = $class->init($arg{config}, $arg{regex});
    return $self;
}

1;

__END__

=head1 NAME

SWISH::3::Analyzer - Swish3 text analyzer class

=head1 SYNOPSIS

 use SWISH::3::Analyzer;
 
 my $analyzer = SWISH::3::Analyzer->new( 
                            config  => SWISH::3::Config->new,
                            regex   => qr/\w+/ 
                            );
                
 my $wordlist = $analyzer->tokenize('some string of words');
 
 # $wordlist is a SWISH::3::Parser::WordList object
 
=head1 DESCRIPTION

SWISH::3::Analyzer is a Perl interface to B<libswish3> tokenizer functions
and implements an object-oriented wrapper around the C<swish_Analyzer>
struct.

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

