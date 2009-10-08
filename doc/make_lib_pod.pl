#!/usr/bin/perl
#
#   read in the libswish3.h header file and format it for the .pod page.
#
#
#

use strict;
use warnings;

sub slurp {
    my $file = shift;
    local $/;
    open(F, "< $file") or die "can't open $file: $!";
    my $buf = <F>;
    close(F);
    return $buf;
}

sub write_file {
    my ($file, $buf) = @_;
    open(F, "> $file") or die "can't write $file: $!";
    print F $buf;
    close(F);
}

# format is:
# /*
# =head2 title
# */
# ......
# /*
# =cut
# */
#
# plus need to add a single space at ^ in order to verbatimize

sub make_pod {
    my $buf = shift;
    my @Pod;
    while ($buf =~ m!^/\*\s+(=head2 .*?\s+)\*/(.*?)/\*\s+=cut\s+\*/!gms)
    {
        my $head = $1;
        my $pod  = $2;
        $pod =~ s,\n,\n ,g;
        push(@Pod, "$head$pod\n");
    }
    return join("\n", @Pod);
}

my $header  = shift(@ARGV) || '../src/libswish3/libswish3.h';
my $tmpl    = shift(@ARGV) || 'libswish3.3.pod.in';
my $pattern = '<<libswish3.h_HERE>>';           # must match .in file
my $buf     = make_pod(slurp($header));
my $pod = slurp($tmpl);
$pod =~ s,$pattern,$buf,;
my $out = $tmpl;
$out =~ s,\.in$,,;
write_file($out, $pod);
