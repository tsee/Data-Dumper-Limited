package Data::Dumper::Limited;
use 5.008;
use strict;
use warnings;
use Carp qw/croak/;
use XSLoader;

our $VERSION = '0.01';

XSLoader::load('Data::Dumper::Limited', $VERSION);

1;

__END__

=head1 NAME

Data::Dumper::Limited - blah

=head1 SYNOPSIS

  use Data::Dumper::Limited

=head1 DESCRIPTION

=head1 AUTHOR

Steffen Mueller E<lt>smueller@cpan.orgE<gt>

Yves Orton E<lt>demerphq@gmail.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Steffen Mueller

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8 or,
at your option, any later version of Perl 5 you may have available.

=cut
