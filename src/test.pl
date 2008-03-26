#!/usr/bin/perl

use strict;
use warnings;

use Test::More tests => 23;

my %docs = (
    'UPPERlower.XML'   => '17',
    'badxml.xml'       => '10',
    'contractions.xml' => '13',
    'foo.txt'          => '16',
    'latin1.xml'       => '3',
    'latin1.txt'       => '3',
    'min.txt'          => '1',
    'multi_props.xml'  => '25',
    'nested_meta.xml'  => '18',
    't.html'           => '6',
    'testutf.xml'      => '8746',    #'8685',
    'utf.xml'          => '30',
    'words.txt'        => '55',
    'words.xml'        => '52',
    'has_nulls.txt'    => '13',
    'no_such_file.txt' => '0',
    'meta.html'        => '23',
    'empty_doc.html'   => '0',
    'no_words.html'    => '0',
    'html_broken.html' => '2',

);

my %stdindocs = (
    'doc.xml' => '8407'

);

for my $file ( sort keys %docs ) {
    cmp_ok( words($file), '==', $docs{$file}, "$file -> $docs{$file} words" );
}

for my $file ( sort keys %stdindocs ) {
    cmp_ok( fromstdin($file), '==', $stdindocs{$file},
        "stdin $file -> $stdindocs{$file} words" );
}

# test header read/write
ok( my $config = slurp('example/swish.xml'), "slurp example/swish.xml" );
system("./swish_header example/swish.xml")
    and die "can't run swish_header: $!";
ok( my $header = slurp('swish_header.xml'), "slurp swish_header.xml" );

# TODO find specific patterns to compare in $config and $header

sub words {
    my $file = shift;
    my $o = join( ' ', `./swish_lint test_docs/$file` );
    my ($count) = ( $o =~ m/nwords: (\d+)/ );
    return $count || 0;
}

sub fromstdin {
    my $file = shift;
    my $o = join( ' ', `./swish_lint - < test_stdin/$file` );
    my ($count) = ( $o =~ m/total words: (\d+)/ );
    return $count || 0;
}

sub slurp {
    my $file = shift;
    local $/;
    open( F, "<$file" ) or die "can't slurp $file: $!";
    my $buf = <F>;
    close(F);
    return $buf;
}

