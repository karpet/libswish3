package SWISH::3::Parser::Data;

use strict;
use warnings;
use Carp;
use Data::Dump qw/ pp /;

our $VERSION = '0.01';

sub handler
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
    for my $d (@SWISH::3::DocFields)
    {

        #printf("%15s: %s\n", $d, $data->doc->$d);
    }

    #print "WordList\n";
    while (my $swishword = $data->wordlist->next)
    {
        for my $w (@SWISH::3::WordFields)
        {

            #printf("%15s: %s\n", $w, $swishword->$w);
        }
    }

}

1;
__END__

=head1 NAME

SWISH::3::Parser::Data - handle the data returned from the SWISH3::3::Parser



=cut
