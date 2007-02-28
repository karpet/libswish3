#!/usr/bin/perl

use strict;
use warnings;
use SWISH::Prog::Headers;
use Search::Tools::XML;

my $usage = "$0 [max_files] [utf_factor]\n";

$ENV{SWISHP} = 1;

# based on
# http://sourceforge.net/mailarchive/message.php?msg_id=10319975
# and modified to make UTF-8 compliant and run under 'use strict'

# Dict file with words. One word per line.
my $dict = "/usr/share/dict/words";

my $min_words_per_file = 3;
my $max_words_per_file = 9999;
my $max_files          = shift @ARGV || 2000;
die $usage if $max_files =~ m/h/i;

my $utf_factor = shift @ARGV;
$utf_factor = 10
  unless
  defined $utf_factor;    # every Nth word gets converted to random UTF string

my $counter = 0;

my ($num_words, @words, $i, $j);

binmode STDOUT, ":utf8";

# Load words
open DICT, "<$dict" or die "can't open $dict: $!\n";

for ($num_words = 0 ; $words[$num_words] = <DICT> ; $num_words++)
{
    chomp $words[$num_words];

    # utf hack: convert every Nth word up a factor of $num_words > 1
    if ($utf_factor > 0 && !$num_words % $utf_factor)
    {
        no bytes;    # so ord() and chr() work as expected
                     #warn ">> $num_words: $words[$num_words]\n";
        my $utf_word = '';
        for my $c (split(//, $words[$num_words]))
        {
            my $u =
              chr(ord($c) + 30000); # 30000 puts it in Chinese range, I think...
            $utf_word .= $u;
        }

        #warn "utf: $utf_word\n";
        $words[$num_words] = $utf_word;
    }
}

close DICT;

srand;

for ($i = 0 ; $i < $max_files ; $i++)
{
    my $this_file_words =
      int(rand($max_words_per_file - $min_words_per_file + 1)) +
      $min_words_per_file;
    my $doc = "";
    for ($j = 0 ; $j < $this_file_words ; $j++)
    {
        $doc .=
          Search::Tools::XML->escape($words[int(rand($num_words - 1))] . " ");
    }
    $doc = <<EOF
<?xml version="1.0" encoding="utf-8"?>
<doc>
$doc
</doc>
EOF
      ;

    #print SWISH::Prog::Headers->head( $doc, { url=>$counter++, mtime=>time(), mime=>'text/xml' } ) . $doc;
    print SWISH::Prog::Headers->head(
                                     $doc,
                                     {
                                      url   => $counter++,
                                      mtime => time(),
                                      mime  => 'text/xml'
                                     }
                                    )
      . $doc;

}
