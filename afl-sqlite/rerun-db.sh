export set AFL_SKIP_CPUFREQ=1
../afl-fuzz -i- -o out -f testdb -T db -- ./fuzzershell --database testdb <db-fuzz-ck.txt
