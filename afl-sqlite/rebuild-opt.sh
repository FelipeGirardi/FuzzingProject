#!/bin/sh
#
# Run this script to rebuild the program used for fuzzing.
#
OBJ="optfuzz"
SRC="sqlite3.c optfuzz.c optfuzz-db01.c"
FILES="$SRC sqlite3.h sqlite3ext.h"
ls -l $FILES
sha1sum $FILES
echo "Building from the files above...."
../afl-gcc -O3 -g -o $OBJ -I. optfuzz.c -ldl
ls -l $OBJ
