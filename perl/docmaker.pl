#!/usr/bin/perl
#
# document maker, using random words from system dictionary
#
# based on
# http://sourceforge.net/mailarchive/message.php?msg_id=10319975
# and modified to make UTF-8 compliant and run under 'use strict'

use strict;
use warnings;
use SWISH::Prog::Headers;
use Search::Tools::XML;
use Term::ProgressBar;
use File::Slurp;
use Path::Class;
use Getopt::Long;

# Dict file with words. One word per line.
my $dict               = "/usr/share/dict/words";
my $max_files          = 1;
my $min_words_per_file = 3;
my $max_words_per_file = 9999;
my $utf_factor         = 10;
my $tmp_dir            = $ENV{TMPDIR} || $ENV{TMP_DIR} || '/tmp';
my $api_version        = $ENV{SWISH_VERSION} || 3;
my $write_indiv_files  = 0;
my $help               = 0;
my $quiet              = 0;
my $segment            = 0;
my $usage              = <<EOF;
$0 [opts]
    --max_files [1]
    --utf_factor [10]
    --write_files
    --min_words [3]
    --max_words [9999]
    --api [3]
    --tmp_dir [/tmp]
    --tmp_dir_segments [0]
    --dictionary [/usr/share/dict/words]
    --quiet
    
EOF

GetOptions(
    'max_files=i'        => \$max_files,
    'min_words=i'        => \$min_words_per_file,
    'max_words=i'        => \$max_words_per_file,
    'api_version=i'      => \$api_version,
    'utf_factor=i'       => \$utf_factor,
    'write_files'        => \$write_indiv_files,
    'help'               => \$help,
    'tmp_dir=s'          => \$tmp_dir,
    'tmp_dir_segments=i' => \$segment,
    'dictionary=s'       => \$dict,
    'quiet'              => \$quiet,

) or die $usage;
die $usage if $help;

my $docmaker = SWISH::Prog::Headers->new( version => $api_version );
my ( $num_words, @words );

binmode STDOUT, ":utf8";

# Load words
open DICT, "<$dict" or die "can't open $dict: $!\n";

for ( $num_words = 0; $words[$num_words] = <DICT>; $num_words++ ) {
    chomp $words[$num_words];

    # utf hack: convert every Nth word up a factor of $num_words > 1
    if ( $utf_factor > 0 && !( $num_words % $utf_factor ) ) {
        no bytes;    # so ord() and chr() work as expected
                     #warn ">> $num_words: $words[$num_words]\n";
        my $utf_word = '';
        for my $c ( split( //, $words[$num_words] ) ) {

            # 30000 puts it in Chinese range, I think...
            my $u = chr( ord($c) + 30000 );
            $utf_word .= $u;
        }

        #warn "utf: $utf_word\n";
        $words[$num_words] = $utf_word;
    }
}

close DICT;

srand;

my $i = 0;
my $progress;
unless ($quiet) {
    $progress
        = Term::ProgressBar->new( { term_width => 80, count => $max_files } );
}

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

    if ( !$write_indiv_files ) {
        my $header = $docmaker->head(
            $xml,
            {   url   => $i,
                mtime => time(),
                mime  => 'text/xml'
            }
        );

        print $header, $xml;
    }
    else {
        my $file = file( $tmp_dir, "$i.xml" );
        if ($segment) {
            my (@dig) = ( $i =~ m/(\d)/g );
            my $len = $#dig;
            $len = $segment if $len > $segment;
            $file = file( $tmp_dir, @dig[ 0 .. $len ], "$i.xml" );
        }
        $file->dir->mkpath;
        write_file( "$file", $xml );
    }

    $progress and $progress->update($i);

}

warn "\n";

