#!/usr/bin/perl -w

print "//generated by interrupt_routines.pl - do not edit\n";
for(my $i = 0; $i < 256; $i++){
    print "void isr_handler_$i(){\n";
    print "write_string_serial(\"Exception $i Occurred\");\n";
    print "}\n\n";

}

