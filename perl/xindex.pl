#!/usr/bin/perl
#
# example xapian indexer
#

use strict;
use warnings;
use Search::Xapian ':all';
use IndexerUtils;
use Getopt::Long;
use Search::Tools::UTF8;

my $index   = 'xapian_index';
my $verbose = 0;
GetOptions( 'index=s' => \$index, 'verbose' => \$verbose );

die "$0 --index <name> files_to_index\n" unless @ARGV;

# these constants should match libswish3.h
my $SWISH_PROP_MTIME_ID = 5;
my $SWISH_PROP_DOCID_ID = 0;

my $db
    = Search::Xapian::WritableDatabase->new( $index, DB_CREATE_OR_OVERWRITE )
    or die "can't create write-able db object: $!\n";

for my $file ( IndexerUtils::aggregate(@ARGV) ) {

    my $uri = "U$file";

    if ( $db->term_exists($uri) ) {

        $verbose and print "$file already in db: skipping ...\n";
        next;

    }

    my $val = 1;
    my $buf = IndexerUtils::normalize( $file, $verbose );
    #$buf = to_utf8( $buf );

    my $doc = Search::Xapian::Document->new
        or die "can't create doc object for $file: $!\n";
    my $analyzer = Search::Xapian::TermGenerator->new;
    $analyzer->set_document($doc);
    $analyzer->index_text($buf);

    # set_data() can be used to store whatever you want
    # but can't be sorted on. so it's like an unsortable property
    $doc->set_data("$file: " . to_utf8( $buf ));

    # add_value() is similar to Swish-e properties
    # results can be sorted by 'value'
    # and each 'value' needs a unique number (like a property id)
    $doc->add_value( $SWISH_PROP_MTIME_ID, time() );    # indexed time
    $doc->add_value( $SWISH_PROP_DOCID_ID, "$file" );

    # add_term() is where you would add a word with no positional info
    # could have META: prefixed however and optional 'weight' as second param
    # we use uri as unique term
    $doc->add_term($uri);

    $db->add_document($doc) or die "failed to add $file: $!";

}
