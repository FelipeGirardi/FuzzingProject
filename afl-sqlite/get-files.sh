#!/bin/sh
#
# Run this script to fetch the source files need to build fuzzershell
#
BASE=/home/drh/sqlite
BLD=$BASE/bld
SRC=$BASE/sqlite
cp $BLD/sqlite3.c $BLD/sqlite3.h $BLD/sqlite3ext.h $BLD/shell.c .
cp $SRC/tool/fuzzershell.c $SRC/ext/misc/json1.c .
