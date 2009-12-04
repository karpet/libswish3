#!/usr/bin/perl

use strict;
use warnings;
use Test::More tests => 29;
use SwishTestUtils;

my $topdir     = $ENV{SVNDIR} || '..';
my $test_docs  = $topdir . "/src/test_docs";
my $test_stdin = $topdir . "/src/test_stdin";
my $os         = $^O;
diag("Testing on '$os'");

my %docs = (
    'UPPERlower.XML'   => '19',
    'badxml.xml'       => '10',
    'contractions.xml' => '17',
    'foo.txt'          => '16',
    'latin1.xml'       => '5',
    'latin1.txt'       => '3',
    'min.txt'          => '1',
    'multi_props.xml'  => '27',
    'nested_meta.xml'  => '20',
    't.html'           => '6',
    'testutf.xml'      => '8756',
    'utf.xml'          => '32',
    'words.txt'        => '55',
    'words.xml'        => '56',
    'has_nulls.txt'    => '13',
    'no_such_file.txt' => '0',
    'meta.html'        => '23',
    'empty_doc.html'   => '0',
    'no_words.html'    => '0',
    'html_broken.html' => '2',
    'properties.html'  => 19,
    'inline.xml'       => 14,
    'inline.html'      => 9,
    'dom.xml'          => 5,

    # these counts are off depending platform, and even then,
    # on which flavor of linux is used.
    # Seems in the case of linux it depends on the glibc implementation
    # of the isw_* functions.

    'UTF-8-demo.txt'       => $os eq 'linux' ? '~7\d\d' : 719,
    'UTF-8-gzipped.txt.gz' => $os eq 'linux' ? '~7\d\d' : 719,
    'utf8-tokens-1.txt'    => $os eq 'linux' ? 12       : 11,

);

my %stdindocs = (
    'doc.xml'  => '8407',
    'test.txt' => 1,

);

for my $file ( sort keys %docs ) {
    my $words    = words($file);
    my $expected = $docs{$file};
    if ( $expected =~ s/^~// ) {
        like( $words, qr/^$expected$/, "$file =~ $expected words" );
    }
    else {
        cmp_ok( $words, '==', $expected, "$file == $expected words" );
    }
}

for my $file ( sort keys %stdindocs ) {
    cmp_ok( fromstdin($file), '==', $stdindocs{$file},
        "stdin $file -> $stdindocs{$file} words" );
}

sub words {
    my $file = shift;
    my $o = join( ' ', `./swish_lint -v $test_docs/$file` );
    my ($count) = ( $o =~ m/nwords: (\d+)/ );
    return $count || 0;
}

sub fromstdin {
    my $file = shift;
    my $o = join( ' ', `./swish_lint -v - < $test_stdin/$file` );
    my ($count) = ( $o =~ m/total words: (\d+)/ );
    return $count || 0;
}

