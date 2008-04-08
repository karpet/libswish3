#!/usr/bin/perl
#
# example xapian searcher
#

use strict;
use warnings;

use Search::Xapian ':all';
use Getopt::Long;
use Search::Tools;
use Search::Tools::UTF8;

my $index = 'xapian_index';
my $uri   = '';
GetOptions( 'index=s' => \$index, 'uri=s' => \$uri );

binmode STDOUT, ':utf8';

die "$0 --index <name> [query | --uri=URI]\n" unless ( @ARGV or $uri );
my $query = join( ' ', map { lc($_) } @ARGV );
my $sb = 1;

print "Searching $index\n";
my $db = Search::Xapian::Database->new($index);

if ($uri) {
    if ( $db->term_exists( 'U' . $uri ) ) {
        my $iter = $db->postlist_begin( 'U' . $uri );
        print "$uri -> " . $iter->get_docid, "\n";
    }
    else {
        print "$uri is not in index\n";
    }

    exit;
}
else {

    my $enq = $db->enquire($query);
    my $regex   = Search::Tools->regexp(query => $query);
    my $snipper = Search::Tools->snipper(query => $regex);
    my $hiliter = Search::Tools->hiliter(query => $regex);

    printf "Running query '%s'\n", $enq->get_query()->get_description();
    my @matches = $enq->matches( 0, 10 );
    print scalar(@matches) . " results found\n";
    foreach my $match (@matches) {
        my $doc = $match->get_document();
        printf "ID %d %d%% [ %s ]\n", $match->get_docid(),
            $match->get_percent(), 
            $hiliter->light( $snipper->snip( to_utf8( $doc->get_data() ) ) );
    }

}
