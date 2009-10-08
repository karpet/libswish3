package SwishTestUtils;
use strict;

my $topdir = $ENV{SVNDIR} || '..';
my $test_docs = "$topdir/src/test_docs";
my $test_configs = "$topdir/src/test_configs";

sub slurp {
    my $file = shift;
    local $/;
    open( F, "<$file" ) or die "can't slurp $file: $!";
    my $buf = <F>;
    close(F);
    return $buf;
}

sub run_lint_stderr {
    my $file   = shift;
    my $config = shift;
    my $cmd
        = $config
        ? "./swish_lint -c $test_configs/$config $test_docs/$file"
        : "./swish_lint $test_docs/$file";
    return run_get_stderr($cmd);

}

sub run_get_stderr {
    my $cmd = shift;
    return join( ' ', `$cmd 2>&1` );
}

1;
