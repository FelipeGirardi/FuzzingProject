#!/bin/sh
#
# Run this script to rebuild the program used for fuzzing.
#
OBJ="fuzzershell"
SRC="sqlite3.c fuzzershell.c json1.c"
FILES="$SRC sqlite3.h sqlite3ext.h"
ls -l $FILES
sha1sum $FILES
echo "Building from the files above...."
OPTS="$OPTS -DSQLITE_THREADSAFE=0"
OPTS="$OPTS -DSQLITE_OMIT_LOAD_EXTENSION"
OPTS="$OPTS -DSQLITE_NO_SYNC"
OPTS="$OPTS -DSQLITE_DEBUG"
OPTS="$OPTS -DSQLITE_OMIT_RANDOMNESS"
OPTS="$OPTS -DSQLITE_ENABLE_JSON1"
OPTS="$OPTS -DSQLITE_ENABLE_DBSTAT_VTAB"
../afl-gcc -O3 -g -o $OBJ $OPTS -I. $SRC -ldl
ls -l $OBJ
gcc -O3 -g -o $OBJ.gcc $OPTS -I. $SRC -ldl
ls -l $OBJ.gcc
