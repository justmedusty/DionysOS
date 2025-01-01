#!/bin/bash
cd ..
echo Lines of code so far including .c files, .h files, and .asm files: 
find -type f -name *.c -o -name *.h -o -name *.asm ! -name "*limine*"  | xargs wc -l | tail -1 | sed 's/[^0-9]*//g'
