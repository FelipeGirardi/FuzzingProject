export set AFL_SKIP_CPUFREQ=1
../afl-fuzz -i- -o cli-out -T cli \
   -x enhanced-sql.dict -- ./sqlite3 --safe
