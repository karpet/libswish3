#!/usr/bin/env perl

use strict;
use warnings;
use Test::More tests => 35;
use SwishTestUtils;

my $topdir     = $ENV{SVNDIR} || '..';
my $test_docs  = $topdir . "/src/test_docs";
my $test_stdin = $topdir . "/src/test_stdin";
my $os         = $^O;
diag("Testing on '$os'");

# .txt files wordcount varies depending on
# the value of $topdir
my $topdir_words = $topdir =~ m/(\w+)/g;
diag("topdir_words=$topdir_words");
my $txt_file_words = $topdir_words + 2;
diag("txt_file_words=$txt_file_words");

my %docs = (
    'badxml.xml'             => '10',
    'contractions.xml'       => '17',
    'dom.xml'                => 5,
    'empty_doc.html'         => '0',
    'foo.txt'                => 18 + $txt_file_words,
    'has_nulls.txt'          => 15 + $txt_file_words,
    'html_broken.html'       => '2',
    'inline.html'            => 9,
    'inline.xml'             => 14,
    'latin1.html'            => 10,
    'latin1-noencoding.html' => 10,
    'latin1.txt'             => '0',                    # under default locale
    'latin1.xml'             => '5',
    'meta.html'              => '23',
    'min.txt'                => 3 + $txt_file_words,
    'multi_props.xml'        => '27',
    'nested_meta.xml'        => '20',
    'no_such_file.txt'       => '0',
    'no_words.html'          => '0',
    'properties.html'        => 19,
    't.html'                 => '6',
    'testutf.xml'            => '8671',
    'UPPERlower.XML'         => '19',
    'utf.xml'                => '32',
    'utf8.html'              => 11,
    'words.txt'              => 57 + $txt_file_words,
    'words.xml'              => '56',
    'xinclude.xml'           => 44 + $txt_file_words,

    # these counts are off depending platform, and even then,
    # on which flavor of linux is used.
    # Seems in the case of linux it depends on the glibc implementation
    # of the isw_* functions.

    'UTF-8-demo.txt' =>
        ( $os eq 'linux' ? '~7\d\d' : ( 723 + $txt_file_words ) ),
    'UTF-8-gzipped.txt.gz' => (
        $os eq 'linux'
        ? '~7\d\d'
        : ( 724 + $txt_file_words )
    ),
    'utf8-tokens-1.txt' => (
        $os eq 'linux' ? ( 16 + $txt_file_words ) : ( 15 + $txt_file_words )
    ),

);

my %stdindocs = (
    'doc.xml' => 8408 + $txt_file_words,

    # isn't really .txt, parsed as XML per header
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

is( words_latin1('latin1.txt'),
    ( 5 + $txt_file_words ),
    "respect SWISH_ENCODING for .txt files"
);
is( words_latin1('greek_and_ojibwe.txt'),
    ( 52 + $txt_file_words ),
    "libxml2 detects encoding, overrides SWISH_ENCODING"
);

for my $file ( sort keys %stdindocs ) {
    cmp_ok( fromstdin($file), '==', $stdindocs{$file},
        "stdin $file -> $stdindocs{$file} words" );
}

sub words {
    my $file = shift;
    my $errors = $ENV{SWISH_DEBUG} ? '' : '2>/dev/null';
    diag("$test_docs/$file");
    my $o = join( ' ', `./swish_lint -v $test_docs/$file $errors` );
    my ($count) = ( $o =~ m/nwords: (\d+)/ );
    return $count || 0;
}

sub words_latin1 {
    $ENV{SWISH_ENCODING} = 'ISO8859-1';
    return words(@_);
}

sub fromstdin {
    my $file = shift;
    diag("$test_stdin/$file");
    my $o = join( ' ', `./swish_lint -v - < $test_stdin/$file` );
    my ($count) = ( $o =~ m/total words: (\d+)/ );
    return $count || 0;
}

