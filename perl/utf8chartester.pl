#!/usr/bin/perl 

use strict;
use warnings;

use POSIX qw(locale_h);
use locale;
use Encode;

# if we set locale explicitly to a latin1-compatible locale
# then \w will match 224 -- otherwise \w misses it
#setlocale(LC_CTYPE, 'en_US' );

my %c = (
    '32'    => 'space',
    '40000' => 'chinese char',
    '90'    => 'Latin Z',
    '160'   => 'nbsp',
    '171'   => 'left arrow',
    '95'    => 'underscore',
    '224'   => 'a with grave accent',
    '97'    => 'Latin a plus grave accent combining mark (2 bytes)'
);

binmode STDOUT, ":utf8";

my $slashw = qr/^\w$/;
my $slashp = qr/^(\p{L}\p{M}*)$/;

for my $n (sort { $a <=> $b } keys %c)
{
    my $c = chr($n);
    if ($n == 97)
    {
        # append grave accent
        # this will turn utf8 flag on where otherwise it would not be
        $c .= chr(768);
    }

    print "$c ($c{$n})\n";
    print "\t", $slashw, ' = ';
    print $c =~ m/$slashw/ ? 1 : 0;
    print "\n";
    print "\t", $slashp, ' = ';
    print $c =~ m/$slashp/ ? 1 : 0;
    print "\n\tUTF8 = ";
    print Encode::is_utf8($c) ? 1 : 0;
    print "\n";
}
