# How to cull test cases

AFL generates a large number of test cases.  But many of the test cases are
redundant in the sense that they do not cause new behavior in the test program.
"Culling" is the process of selecting a subset of the test cases which cover
all behavior but which are also much smaller than the total set.

This document overviews the steps for culling the test cases.

  1.  Create two new directories, one in which to store the complete set
      of test cases and another in which to store the culled output.

         mkdir tmp1 cull1

  2.  Copy test cases into tmp1.  The example below assumes that afl-fuzz
      has been run (or is running) with "-o out".

         cp out/*/id:* tmp1

  3.  Run afl-cmin on to generate the culled output.  Two examples are
      below.  The first example is for database fuzzing and the second
      is for SQL fuzzing.  In the first example, take care that the argument
      to -f is different from the -f argument of a running ../afl-fuzz.

         ../afl-cmin -i tmp1 -o cull1 -f culldb -- ./fuzzershell --database culldb db-fuzz-ck.txt

         ../afl-cmin -t 20 -i tmp1 -o cull1 -- ./fuzzershell

      Note that the timeout option (-t) is omitted from the database fuzz
      cases to help capture cases that cause infinite loops.  But a timeout
      is included in SQL fuzz cases to suppress SQL statements (such as 
      unbounded recursive common table expressions) that really are suppose 
      to run forever.

# Loading culled content into fuzzdataN.db files

After culling test cases, they can to be loaded into one of the test/fuzzdataN.db
files of the [main SQLite source repository](http://www.sqlite.org/src) so that they
are rerun during normal testing.  For SQL tests, create the new database as follows:

         fuzzcheck f1.db --load-sql cull1/*
         fuzzcheck f1.db -m 'Comment here'

For Database tests, use the following commands:

         fuzzcheck f1.db --load-sql db-fuzz-ck.txt
         fuzzcheck f1.db --load-db cull1/*
         fuzzcheck f1.db -m 'Comment here'

After creating the new database f1.db (and testing it using "fuzzcheck f1.db")
copy it into the appropriately named fuzzdataN.db file in test/ of the main
repository and commit.
