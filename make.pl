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

unless ($#ARGV > -1) {
    make_all();
}
else{
    if($ARGV[0] eq "clean"){
        clean();
    }
    elsif($ARGV[0] eq "libs"){
        libs();
    }
    elsif($ARGV[0] eq "all"){
        make_all();
    }
}

sub make_all{
    libs();
    build();
}

sub libs{
    `mkdir -p lib`;
    chdir "src";
    `gcc -c *.c`;
    `ar rcs libcnf_parse.a *.o`;
    `rm *.o`;
    `mv *.a ../lib`;
    chdir "../";
}

sub build{
    `mkdir -p bin`;
    my @libs = `ls lib`;
    my $lib_string = "";
    for(@libs){ chomp $_; $lib_string .= "lib/$_ " };
    my $build_string =
        "gcc test/main.c $lib_string -Isrc -o bin/test";
    `$build_string`;
}

sub clean{
    `rm -rf lib`;
    `rm -rf bin`;
}


