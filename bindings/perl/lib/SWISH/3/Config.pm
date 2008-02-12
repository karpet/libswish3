package SWISH::3::Config;
use strict;
use warnings;
use Carp;
use SWISH::3;
#use SWISH::3::Constants;

our $VERSION = '0.01';

sub properties {
    $_[0]->subconfig( SWISH::3::Constants::SWISH_PROP() );
}

sub metanames {
    $_[0]->subconfig( SWISH::3::Constants::SWISH_META() );
}

sub mimes {
    $_[0]->subconfig( SWISH::3::Constants::SWISH_MIME() );
}

sub parsers {
    $_[0]->subconfig( SWISH::3::Constants::SWISH_PARSERS() );
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

Copyright 2008 by Peter Karman

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself. 

=cut


