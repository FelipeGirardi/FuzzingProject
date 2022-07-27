#!/bin/sh
#
# Run this to recompile for session fuzzing.
#
../afl-gcc -O3 -g -I. -o sessionfuzz sessionfuzz.c -lz
ls -l sessionfuzz
