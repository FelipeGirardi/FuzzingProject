#!/bin/bash
set -e

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
# - env TARGET: path to target work dir
# - env MAGMA: path to Magma support files
# - env OUT: path to directory where artifacts are stored
# - env CFLAGS and CXXFLAGS must be set to link against Magma instrumentation
##

export CC="$FUZZER/repo/aflplusplus/afl-clang-fast"
export CXX="$FUZZER/repo/aflplusplus/afl-clang-fast++"
export AS="llvm-as"

export LIBS="$LIBS -lc++ -lc++abi $FUZZER/repo/aflplusplus/utils/aflpp_driver/libAFLDriver.a"

export CXXFLAGS="$CXXFLAGS -stdlib=libc++"

# Build the AFL-only instrumented version
(
    export AFL_LLVM_INSTRUMENT="classic"
    export OUT="$OUT/afl"
    export LDFLAGS="$LDFLAGS -L$OUT"

    "$MAGMA/build.sh"
    "$TARGET/build.sh"
)

# NOTE: We pass $OUT directly to the target build.sh script, since the artifact
#       itself is the fuzz target. In the case of Angora, we might need to
#       replace $OUT by $OUT/fast and $OUT/track, for instance.
