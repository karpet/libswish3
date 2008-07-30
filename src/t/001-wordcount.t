#!/usr/bin/perl

use strict;
use warnings;
use Test::More tests => 25;
use SwishTestUtils;

my %docs = (
    'UPPERlower.XML'   => '19',
    'badxml.xml'       => '10',
    'contractions.xml' => '13',
    'foo.txt'          => '16',
    'latin1.xml'       => '3',
    'latin1.txt'       => '3',
    'min.txt'          => '1',
    'multi_props.xml'  => '25',
    'nested_meta.xml'  => '18',
    't.html'           => '6',
    'testutf.xml'      => '8754',
    'utf.xml'          => '30',
    'words.txt'        => '55',
    'words.xml'        => '52',
    'has_nulls.txt'    => '13',
    'no_such_file.txt' => '0',
    'meta.html'        => '23',
    'empty_doc.html'   => '0',
    'no_words.html'    => '0',
    'html_broken.html' => '2',
    'properties.html'  => 19,
    'inline.xml'       => 12,
    'inline.html'      => 9,

);

my %stdindocs = (
    'doc.xml'  => '8407',
    'test.txt' => 1,

);

for my $file ( sort keys %docs ) {
    cmp_ok( words($file), '==', $docs{$file}, "$file -> $docs{$file} words" );
}

for my $file ( sort keys %stdindocs ) {
    cmp_ok( fromstdin($file), '==', $stdindocs{$file},
        "stdin $file -> $stdindocs{$file} words" );
}

sub words {
    my $file = shift;
    my $o = join( ' ', `./swish_lint -t test_docs/$file` );
    my ($count) = ( $o =~ m/nwords: (\d+)/ );
    return $count || 0;
}

sub fromstdin {
    my $file = shift;
    my $o = join( ' ', `./swish_lint -t - < test_stdin/$file` );
    my ($count) = ( $o =~ m/total words: (\d+)/ );
    return $count || 0;
}

