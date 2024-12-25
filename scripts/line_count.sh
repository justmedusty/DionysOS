#!/bin/bash
cd ..
echo Lines of code so far: 
find -type f -name *.c ! -name "*limine*" | xargs wc -l | tail -1 | sed 's/[^0-9]*//g'
