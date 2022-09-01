#!/bin/bash

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env SHARED: path to directory shared with host (to store results)
# - env PROGRAM: name of program to run (should be found in $OUT)
# - env ARGS: extra arguments to pass to the program
# - env FUZZARGS: extra arguments to pass to the fuzzer
##

mkdir -p "$SHARED/findings" "$SHARED/output"
cd "$FUZZER/repo/superion/tree_mutation"

cmake ./
make

cd js_parser/
for f in *.cpp; do g++ -I ../runtime/src/ -c $f -fPIC -std=c++11; done
g++ -shared -std=c++11 *.o ../dist/libantlr4-runtime.a  -o libTreeMutation.so

cd ../../llvm_mode/
LLVM_CONFIG=llvm-config-3.8 CXXFLAGS="-DLLVM38" make

cd ../
make

wget https://github.com/php/php-src/archive/master.zip
export CC=~/path_to_Superion/afl-gcc
export CXX=~/path_to_Superion/afl-g++
export AFL_HARDEN=1
./buildconf
./configure
make

./afl-fuzz -M f1 -m 1G -t 100+ -i ~/phpseeds/ -o ~/phpout/ ~/php-src-master/sapi/cli/php @@
./afl-fuzz -S f2 -m 1G -t 100+ -i ~/phpseeds/ -o ~/phpout/ ~/php-src-master/sapi/cli/php @@
./afl-fuzz -S f3 -m 1G -t 100+ -i ~/phpseeds/ -o ~/phpout/ ~/php-src-master/sapi/cli/php @@
./afl-fuzz -S f4 -m 1G -t 100+ -i ~/phpseeds/ -o ~/phpout/ ~/php-src-master/sapi/cli/php @@
