use strict;
use warnings;
use Data::Dumper::Limited qw(DumpLimited);
use Scalar::Util qw(weaken);

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
undef $r->[0];

$r = [[[\$r]]];
ok(not(eval {DumpLimited($r); 1}) && $@, "deep cyclic refs barf");
undef $r->[0];

$x = [];
$r = [$x, $x];
is(DumpLimited($r), "[[],[]]", "multiple identical refs (2)");

weaken($r->[1]);
is(DumpLimited($r), "[[],[]]", "multiple identical refs (2)");

$r = [[\$r, \$r]];
weaken($r->[0][0]);
ok(not(eval {DumpLimited($r); 1}) && $@, "deep cyclic refs barf, even with weakrefs");
undef $r->[0];

$r = [$x, $x];
ok(not(eval {DumpLimited($r, {disallow_multi => 1}); 1}) && $@, "non-cyclic, repeated refs barf under disallow_multi");


done_testing();
