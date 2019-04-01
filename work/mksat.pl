#!/usr/bin/perl

use strict;

my $sat_cmdline=<<"END_SAT_CMDLINE";
gcc 
test/main.c 
/home/nomina/ckbauman/sat_tools/lib/libsat_tools.a 
/home/nomina/ckbauman/cnf_parse/lib/libcnf_parse.a 
/home/nomina/ckbauman/lib_babel/lib/libbabel.a   
-Wall -Wfatal-errors 
-lm -lncurses -lpthread -lrt
-I/home/nomina/ckbauman/sat_tools/src 
-I/home/nomina/ckbauman/cnf_parse/src 
-I/home/nomina/ckbauman/lib_babel/src  
-L/home/nomina/ckbauman/sat_tools/lib 
-L/home/nomina/ckbauman/cnf_parse/lib 
-L/home/nomina/ckbauman/lib_babel/lib  
-o bin/test
END_SAT_CMDLINE

my @sat_cmdline = split /\n/, $sat_cmdline;

my @grep_lines = grep( !/^#/, @sat_cmdline);

$sat_cmdline = join(' ', @grep_lines);

#print STDERR "MKSAT.PL|\t$sat_cmdline\n";
print STDOUT $sat_cmdline;
 

