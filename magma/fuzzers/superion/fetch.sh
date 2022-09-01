#!/bin/bash
set -e

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
##

git clone --no-checkout https://github.com/AFLplusplus/AFLplusplus "$FUZZER/repo/aflplusplus"
git -C "$FUZZER/repo/aflplusplus" checkout 458eb0813a6f7d63eed97f18696bca8274533123

# Fix: CMake-based build systems fail with duplicate (of main) or undefined references (of LLVMFuzzerTestOneInput)
sed -i '{s/^int main/__attribute__((weak)) &/}' $FUZZER/repo/aflplusplus/utils/aflpp_driver/aflpp_driver.c
sed -i '{s/^int LLVMFuzzerTestOneInput/__attribute__((weak)) &/}' $FUZZER/repo/aflplusplus/utils/aflpp_driver/aflpp_driver.c
cat >> $FUZZER/repo/aflplusplus/utils/aflpp_driver/aflpp_driver.c << EOF
__attribute__((weak))
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
  // assert(0 && "LLVMFuzzerTestOneInput should not be implemented in afl_driver");
  return 0;
}
EOF

patch -p1 -d "$FUZZER/repo/aflplusplus" << EOF
--- a/utils/aflpp_driver/aflpp_driver.c
+++ b/utils/aflpp_driver/aflpp_driver.c
@@ -53,7 +53,7 @@
   #include "hash.h"
 #endif
 
-int                   __afl_sharedmem_fuzzing = 1;
+int                   __afl_sharedmem_fuzzing = 0;
 extern unsigned int * __afl_fuzz_len;
 extern unsigned char *__afl_fuzz_ptr;
 
@@ -111,7 +111,8 @@ extern unsigned int * __afl_fuzz_len;
 __attribute__((weak)) int LLVMFuzzerInitialize(int *argc, char ***argv);
 
 // Notify AFL about persistent mode.
-static volatile char AFL_PERSISTENT[] = "##SIG_AFL_PERSISTENT##";
+// DISABLED to avoid afl-showmap misbehavior
+static volatile char AFL_PERSISTENT[] = "##SIG_AFL_NOT_PERSISTENT##";
 int                  __afl_persistent_loop(unsigned int);
 
 // Notify AFL about deferred forkserver.
EOF

git clone --no-checkout 'https://github.com/zhunki/Superion' "$FUZZER/repo/superion"
git -C "$FUZZER/repo/superion" checkout 4b3d49afd50ce426c0b05c3f4d9fb73d8fd2631b
