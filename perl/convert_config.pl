#!/usr/bin/perl -w
#
# convert swish 2 config to swish 3 (xml) config
#

use strict;
use SWISH::Prog::Config;
use File::Slurp;

my $usage = "$0 config_file [...config_fileN]\n";

die $usage unless @ARGV;

my $config = SWISH::Prog::Config->new;
for my $f (@ARGV)
{
    my $xml = $config->ver2_to_ver3($f);

    my $new = "$f.xml";

    write_file($new, $xml);
}
