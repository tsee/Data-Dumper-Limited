use strict;
use warnings;
use Data::Dumper::Limited qw(DumpLimited);

use Test::More;

# silly thing, but tests that it doesn't crash and burn
is(DumpLimited(undef), "undef");

is(DumpLimited(1), "1");
is(DumpLimited(-1), "-1");
is(DumpLimited(2), "2");
is(DumpLimited(2e10), "2". ("0" x 10));
is(DumpLimited(99999.9881), "99999.9881");
is(DumpLimited(-2.1111), "-2.1111");

TODO: {
  local $TODO = "Strings not implemented yet";
  is(DumpLimited("foo"), '"foo"');
}

pass();
done_testing();
