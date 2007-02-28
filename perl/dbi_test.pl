use lib qw( /usr/local/lib/swish-e/perl );

package My::DBI;
use base qw( SWISH::Prog::DBI );

use Encode;
use Search::Tools::Transliterate;

sub title_filter
{
    my $self = shift;
    my $row  = shift;

    return $row->{Movie_Title};

}

sub row_filter
{
    my $self = shift;
    my $row  = shift;

    for (keys %$row)
    {

        # until we store as proper UTF8 we need this
        if (!Encode::is_utf8($row->{$_}))
        {
            $row->{$_} =
              Search::Tools::Transliterate->convert(
                Encode::encode_utf8(Encode::decode('MacRoman', $row->{$_}, 1)));

        }
    }

}

1;

package My::DBI::Doc;
use base qw( SWISH::Prog::DBI::Doc );

my $url_base = 'http://al.com/play/';

sub url_filter
{
    my $doc     = shift;
    my $db_data = $doc->row;

    $doc->url($url_base . $db_data->{ID});
}

sub content_filter
{
    my $doc = shift;
    my $c   = $doc->content;

    # get rid of the timer marks in the Caption field
    $c =~ s,\[\d\d:\d\d:\d\d\.\d\d\],,g;

    #warn $c;

    $doc->content($c);
}

1;

package main;
use Carp;
use Data::Dumper;

my $dbi_indexer = My::DBI->new(
                    db => [
                           "DBI:mysql:database=movies;host=localhost;port=3306",
                           'aluser', 'alpass',
                           {
                            RaiseError  => 1,
                            HandleError => sub { confess(shift) },
                           }
                          ],
                    name  => 'movielist.index',
                    #fh    => 0,    # print to stdout
                    #quiet => 1,     # don't print counter (with fh=>0)
                    #debug => 1,    # print config etc.
);

$dbi_indexer->create(
    tables => {
        'movielist' => {
            columns => {
                        Movie_Title => 1,
                        Application => 1,
                        Description => 1,
                        Question    => 1,
                        Caption     => 1,
                        ID          => 1,
                       },
            title => 'Movie_Title',
            desc  => {
                Description => 1,
                Caption     => 1,

                #Question    => 1
                    }
        }
    }
);

exit;

