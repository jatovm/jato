#! /usr/bin/perl

use strict;
use warnings;

my %opc;

for (<>) {
	next unless m/(\d+) \((0x[A-Za-z0-9]+)\) (\w+)/;
	my $mnemonic = uc $3;
	my $value = $1;
	my $hex_value = oct $2;

	if (exists $opc{$mnemonic} && $opc{$mnemonic} != $hex_value) {
		printf STDERR "%s defined twice: %d and %d\n",
			$mnemonic, $opc{$mnemonic}, $hex_value;
	}

	$opc{$mnemonic} = $hex_value;
}

print "#ifndef __VM_OPCODES_H\n";
print "#define __VM_OPCODES_H\n";
print "\n";
print "/* Note: This file was generated automatically. DO NOT EDIT! */\n";
print "\n";

my $fmt = "#define OPC_%s";

my $maxpos = 0;
for my $mnemonic (keys %opc) {
	my $pos = length sprintf $fmt, $mnemonic;
	$maxpos = $pos if $pos > $maxpos;
}

# round upwards to the nearest tab-stop
$maxpos = 8 + ($maxpos & ~7);

for my $mnemonic (sort { $opc{$a} <=> $opc{$b} } keys %opc) {
	my $pos = length sprintf $fmt, $mnemonic;
	$pos = 8 + ($pos & ~7);

	printf $fmt, $mnemonic;
	for (my $i = 0; $i < 1 + ($maxpos - $pos) / 8; ++$i) {
		print "\t";
	}
	printf "0x%02x\n", $opc{$mnemonic};
}

print "\n";
print "#endif\n";
