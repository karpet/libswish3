package SwishTestUtils;

sub slurp {
    my $file = shift;
    local $/;
    open( F, "<$file" ) or die "can't slurp $file: $!";
    my $buf = <F>;
    close(F);
    return $buf;
}

sub run_get_stderr {
    my $file   = shift;
    my $config = shift;
    my $cmd
        = $config
        ? "./swish_lint -c test_configs/$config test_docs/$file 2>&1"
        : "./swish_lint test_docs/$file 2>&1";
    return join( ' ', `$cmd` );

}

1;
