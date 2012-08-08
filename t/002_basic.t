use strict;
use warnings;
use Data::Dumper::Limited qw(DumpLimited);

use Test::More;

# silly thing, but tests that it doesn't crash and burn
is(DumpLimited(undef), "undef");

is(DumpLimited(1), "1", "integer");
is(DumpLimited(-1), "-1", "negative int");
is(DumpLimited(2), "2", "integer (2)");
is(DumpLimited(2e10), "2". ("0" x 10), "large integer");
is(DumpLimited(99999.9881), "99999.9881", "float");
is(DumpLimited(-2.1111), "-2.1111", "negative float");

TODO: {
  local $TODO = "Strings not implemented yet";
  is(DumpLimited("foo"), '"foo"', "string");
}

my $x = 2.1;
is(DumpLimited($x), "2.1", "variable");
is(DumpLimited(\2.1), "\\2.1", "scalar reference");
is(DumpLimited(\$x), "\\2.1", "scalar reference (2)");

is(DumpLimited([]), "[]", "empty array");
is(DumpLimited([1, 2.1]), "[1,2.1]", "array with numbers");
is(DumpLimited([[[],[]],[]]), "[[[],[]],[]]", "nested arrays");

is(DumpLimited({"" => [1,2,3]}), q!{"",[1,2,3]}!, "hash with empty string key");
TODO: {
  local $TODO = "Strings not implemented yet";
  is(DumpLimited({"aaa" => [1,2,3]}), q!{"aaa",[1,2,3]}!, "hash with simple key");
}

is(DumpLimited([\$x, \$x]), "[\\2.1,\\2.1]", "multiple identical refs");

my $r;
$r = [\$r];
ok(not(eval {DumpLimited($r); 1}) && $@, "cyclic refs barf");

pass();
done_testing();
