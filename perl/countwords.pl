#!/usr/bin/perl -w
#
# count instances of words in a swish-e index
# and report on NUM number of top instances
#
# usage: countwords [NUM [INDEX]]
#
# Copyright 2005 karpet@peknet.com
# released under same terms as Perl
#
# TODO: swish-e ought to contain this feature natively
#

use strict;
use Text::FormatTable;

my ($num,$index) = @ARGV;

#defaults
$index ||= 'index.swish-e';
$num   ||= 50;

unless( -s $index )
{
    die "no such index: $index\n";
}
unless( $num )
{
    die "need a number of words to output\n";
}

my $count;
my $cmd = "swish-e -f $index -T INDEX_WORDS";

warn $cmd, $/;

open(SWISH, "$cmd |")
        or die "can't exec '$cmd': $!\n";
        
while(<SWISH>) {
        chomp;
        my ($word,@insts) = split /\[\d+ /, $_ ;
        INST: for my $i (@insts) {
                next INST if ! $i;
                my ($doc,$cnt) = split(/\s+/,$i);
                $count->{$word}->[0] += $cnt;
                $count->{$word}->[1]++;
        }
}

close(SWISH);

# print results, stopping at $num
# use FormatTable for pretty ASCII

my $tbl = new Text::FormatTable('r  r  l  l');
$tbl->head('#','word','count','unique docs');
$tbl->rule('=');
my $seen = 0;
my $n = 0;

for my $word (sort { 
        $count->{$b}->[0] <=> $count->{$a}->[0]
        } keys %$count) {
        my ($cnt,$docs) = @{ $count->{$word} };
        $tbl->row(++$n, $word, $cnt, $docs);
        last if ++$seen == $num;
}

print $tbl->render(60);

