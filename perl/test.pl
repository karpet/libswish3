#!/usr/bin/perl

use SWISH::Prog::Headers;
binmode STDOUT, ":utf8";

my $string = "\x{263a}\x{263a}\x{263a}\x{263a}\x{263a}";
my $doc = <<EOF;
<?xml version="1.0" encoding="utf-8"?>
<doc>
1234 56789 0 $string
</doc>
EOF

print SWISH::Prog::Headers->head(
                                 $doc,
                                 {
                                  url   => 'somedoc',
                                  mtime => time(),
                                  mime  => 'text/xml'
                                 }
                                )
  . $doc;
