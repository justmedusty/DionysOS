#!/bin/bash

if [[ ! -e ./line_count.sh ]]; then
  echo "This script must be run from the actual directory the script resides in!"
  exit
fi
cd ..
echo Lines of code so far including .c files, .h files, and .asm files: 
find -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -not -name "*limine*" | xargs wc -l | tail -1 | sed 's/[^0-9]*//g'
