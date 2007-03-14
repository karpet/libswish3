package SWISH::3::Constants;
use strict;
use warnings;
use base qw( Exporter );
use SWISH::3;

use constant SWISH_DOC_FIELDS =>
  qw( mtime size encoding mime uri nwords ext parser );
use constant SWISH_WORD_FIELDS =>
  qw( word position metaname context start_offset end_offset );

# our symbol table is populated with newCONSTSUB in 3.xs
# directly from libswish3.h, so we just grep the symbol table.
my @consts = grep { m/^SWISH_/ } keys %SWISH::3::Constants::;

our @EXPORT = (
    qw(
      SWISH_DOC_FIELDS
      SWISH_WORD_FIELDS
      ),
    @consts
);

1;
