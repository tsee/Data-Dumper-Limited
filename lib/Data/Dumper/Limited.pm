package Data::Dumper::Limited;
use 5.008;
use strict;
use warnings;
use Carp qw/croak/;
use XSLoader;

our $VERSION = '0.01';
use Exporter 'import';
our @EXPORT_OK = qw(DumpLimited);
our %EXPORT_TAGS = (all => \@EXPORT_OK);

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

Some inspiration and code was taken from Marc Lehmann's
excellent JSON::XS module due to obvious overlap in
problem domain.

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Steffen Mueller

Except portions taken from Marc Lehmann's code for the JSON::XS
module. The license for JSON::XS is the same as for this module:

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut
