package IndexerUtils;
use strict;
use warnings;
use Carp;
use File::Find;
use File::Slurper qw( read_text );

sub aggregate {
    my @where = @_;
    my $Ext   = qr{html?|sgml?|xml|txt}i;
    my @filenames;

    find(
        {   wanted => sub {
                return unless $_ =~ m/\.$Ext$/;

                push( @filenames, $_ );
            },
            no_chdir => 1
        },
        @where
    );
    return @filenames;
}

sub normalize {
    my $file = shift or croak "file required";
    my $verbose = shift || 0;

    $verbose and print "indexing $file ...\n";
    my $buf = read_text($file);

    # strip any markup
    $buf =~ s,<.+?>,,sg;
    return $buf if !wantarray;

    # naive tokenizer
    my @w = grep {m/./} split( /\s+/, $buf );

    $verbose and print scalar(@w), " words in $file\n";
    return @w;
}

1;
