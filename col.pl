use strict;
use warnings;

while (<>) {  # Read each line from the file
    my @fields = split;  # Split into fields based on whitespace

    next unless @fields >= 14; # Ensure at least 14 fields exist

    my $column4 = $fields[3];
    my $column9 = $fields[8];
    my $tlb_misses = $fields[13];

    # Remove the trailing comma from tlb_misses
    $tlb_misses =~ s/,//; 
    $tlb_misses =~ s/misses=//; 

    print "$column4 $column9 $tlb_misses\n";
}
