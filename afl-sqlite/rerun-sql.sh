export set AFL_SKIP_CPUFREQ=1
../afl-fuzz -i- -o out -T sql -x ../testcases/_extras/sql.dict -- ./fuzzershell
