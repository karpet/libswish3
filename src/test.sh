#!/bin/sh -x

PERL_DL_NONLAZY=1 perl "-MExtUtils::Command::MM" "-e" "test_harness(0, 't')" t/*.t
