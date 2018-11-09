# make.pl
#
# Use this to build zebra on any standard *nix with GCC
#
# Use at your own risk. I use Perl instead of autotools because the latter
#    are an opaque morass of Mad Hatter gibberish.
#
#   USAGE:
#
#   Build all:
#       perl make.pl
#
#   Clean:
#       perl make.pl clean
#$verbose=1;

unless ($#ARGV > -1) {
    make_all();
}
else{
    if($ARGV[0] eq "clean"){
        clean();
    }
    elsif($ARGV[0] eq "clean_all"){
        clean_all();
    }
    elsif($ARGV[0] eq "lib_babel"){
        lib_babel();
    }
    elsif($ARGV[0] eq "all_libs"){
        all_libs();
    }
    elsif($ARGV[0] eq "libs"){
        libs();
    }
    elsif($ARGV[0] eq "all"){
        make_all();
    }
}

sub make_all{
    print "make_all\n" if $verbose;
    all_libs();
    build();
}

sub all_libs{
    print "all_libs\n" if $verbose;
    `mkdir -p lib`;
    lib_babel();
#    cnf_parse();
    libs();
}

sub lib_babel{
    print "lib_babel\n" if $verbose;
    chdir "lib_babel";
    `perl make.pl libs`;
    chdir "../";
    `cp lib_babel/lib/libbabel.a lib`;
}

#sub cnf_parse{
#    print "cnf_parse\n" if $verbose;
#    chdir "cnf_parse";
#    `perl make.pl libs`;
#    chdir "../";
#    `cp cnf_parse/lib/libcnf_parse.a lib`;
#}

sub libs{
    print "libs\n" if $verbose;
    `mkdir -p lib`;
    chdir "src";
    `gcc -c *.c -I../lib_babel/src -Wfatal-errors -lm`;
#    `gcc -O3 -c *.c -I../lib_babel/src -I../cnf_parse/src -Wfatal-errors -lm`;
    `ar rcs libsat_tools.a *.o`;
    `mv *.o ../lib`;
    `mv *.a ../lib`;
    chdir "../";
}

sub build{
    print "build\n" if $verbose;
    `mkdir -p bin`;
    my @libs = `ls lib`;
    my $lib_string = "";
    for(@libs){ chomp $_; $lib_string .= "lib/$_ " };
    my $build_string =
        "gcc test/main.c $lib_string -lbabel -Llib -Isrc -Ilib_babel/src -o bin/test -Wfatal-errors -lm";
#        "gcc -O3 test/main.c $lib_string -Isrc -Ilib_babel/src -Icnf_parse/src -o bin/test -Wfatal-errors -lm";
    `$build_string`;
}

sub clean_all{
    print "clean_all\n" if $verbose;
    chdir "lib_babel";
    `perl make.pl clean`;
#    chdir "../cnf_parse";
#    `perl make.pl clean`;
    chdir "../";
    clean();
}

sub clean{
    print "clean\n" if $verbose;
    `rm -rf lib`;
    `rm -rf bin`;
}

