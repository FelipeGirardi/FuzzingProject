export set AFL_SKIP_CPUFREQ=1
../afl-fuzz -i upsert-cases -o upsert-out -T upsert \
   -x enhanced-sql.dict -- ./fuzzershell
