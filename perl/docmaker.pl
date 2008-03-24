#!/usr/bin/perl

use strict;
use warnings;
use SWISH::Prog::Headers;
use Search::Tools::XML;
use Term::ProgressBar;

my $usage = "$0 [max_files] [utf_factor]\n";

die $usage unless @ARGV;

my $docmaker = SWISH::Prog::Headers->new;

#$ENV{SWISH3} = 1;

# based on
# http://sourceforge.net/mailarchive/message.php?msg_id=10319975
# and modified to make UTF-8 compliant and run under 'use strict'

# Dict file with words. One word per line.
my $dict = "/usr/share/dict/words";

my $min_words_per_file = 3;
my $max_words_per_file = 9999;
my $max_files          = shift @ARGV;
die $usage if $max_files =~ m/h/i;

my $utf_factor = shift @ARGV;
$utf_factor = 10
    unless
    defined $utf_factor;  # every Nth word gets converted to random UTF string

my ( $num_words, @words );

binmode STDOUT, ":utf8";

# Load words
open DICT, "<$dict" or die "can't open $dict: $!\n";

for ( $num_words = 0; $words[$num_words] = <DICT>; $num_words++ ) {
    chomp $words[$num_words];

    # utf hack: convert every Nth word up a factor of $num_words > 1
    if ( $utf_factor > 0 && !$num_words % $utf_factor ) {
        no bytes;    # so ord() and chr() work as expected
                     #warn ">> $num_words: $words[$num_words]\n";
        my $utf_word = '';
        for my $c ( split( //, $words[$num_words] ) ) {
            my $u = chr( ord($c) + 30000 )
                ;    # 30000 puts it in Chinese range, I think...
            $utf_word .= $u;
        }

        #warn "utf: $utf_word\n";
        $words[$num_words] = $utf_word;
    }
}

close DICT;

srand;

my $i = 0;
my $progress
    = Term::ProgressBar->new( { term_width => 80, count => $max_files } );

# preallocate memory (doesn't really matter after all...)
my $doc = ' ' x ( $max_words_per_file * 10 );
my $xml = $doc;
while ( $i++ < $max_files ) {
    my $this_file_words
        = int( rand( $max_words_per_file - $min_words_per_file + 1 ) )
        + $min_words_per_file;
    $doc = '';
    my $word_cnt = 0;
    while ( $word_cnt++ < $this_file_words ) {
        $doc .= $words[ int( rand( $num_words - 1 ) ) ] . ' ';
    }
    Search::Tools::XML->escape($doc);
    $xml = qq(<?xml version="1.0" encoding="utf-8"?>
<doc>
$doc
</doc>
);

    my $header = $docmaker->head(
        $xml,
        {   url   => $i,
            mtime => time(),
            mime  => 'text/xml'
        }
    );

    print $header, $xml;

    $progress->update($i);

}

warn "\n";

