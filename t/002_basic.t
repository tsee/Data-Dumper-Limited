use strict;
use warnings;
use Data::Dumper::Limited qw(DumpLimited);

use Test::More;

# silly thing, but tests that it doesn't crash and burn
is(DumpLimited(undef), "undef");

pass();
done_testing();
