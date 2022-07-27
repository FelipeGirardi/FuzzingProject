/*
** 2018-03-21
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This program attempts to verify the correctness of the SQLite query
** optimizer by fuzzing.
**
** The input is an SQL script, presumably generated by a fuzzer.  The
** argument is the name of the input.  If no files are named, standard
** input is read.
**
** The SQL script is run twice, once with optimization enabled, and again
** with optimization disabled.  If the output is not equivalent, an error
** is printed and the program returns non-zero.
*/

/* Include the SQLite amalgamation, after making appropriate #defines.
*/
#define SQLITE_THREADSAFE 0
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_ENABLE_DESERIALIZE 1
#include "sqlite3.c"

/* Content of the read-only test database */
#include "optfuzz-db01.c"

/*
** Prepare a single SQL statement.  Panic if anything goes wrong
*/
static sqlite3_stmt *prepare_sql(sqlite3 *db, const char *zFormat, ...){
  char *zSql;
  int rc;
  sqlite3_stmt *pStmt = 0;
  va_list ap;

  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
  if( rc ){
    printf("Error: %s\nSQL: %s\n",
           sqlite3_errmsg(db), zSql);
    exit(1);
  }
  sqlite3_free(zSql);
  return pStmt;
}

/*
** Run SQL.  Panic if anything goes wrong
*/
static void run_sql(sqlite3 *db, const char *zFormat, ...){
  char *zSql;
  int rc;
  char *zErr = 0;
  va_list ap;

  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  rc = sqlite3_exec(db, zSql, 0, 0, &zErr);
  if( rc || zErr ){
    printf("Error: %s\nsqlite3_errmsg: %s\nSQL: %s\n",
           zErr, sqlite3_errmsg(db), zSql);
    exit(1);
  }
  sqlite3_free(zSql);
}

/*
** Run one or more SQL statements contained in zSql against database dbRun.
** Store the input in database dbOut.
*/
static int optfuzz_exec(
  sqlite3 *dbRun,             /* The database on which the SQL executes */
  const char *zSql,           /* The SQL to be executed */
  sqlite3 *dbOut,             /* Store results in this database */
  const char *zOutTab,        /* Store results in this table of dbOut */
  int *pnStmt,                /* Write the number of statements here */
  int *pnRow,                 /* Write the number of rows here */
  int bTrace                  /* Print query results if true */
){
  int rc = SQLITE_OK;         /* Return code */
  const char *zLeftover;      /* Tail of unprocessed SQL */
  sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
  sqlite3_stmt *pIns = 0;     /* Statement to insert into dbOut */
  const char *zCol;           /* Single column value */
  int nCol;                   /* Number of output columns */
  char zLine[4000];           /* Complete row value */

  run_sql(dbOut, "BEGIN");
  run_sql(dbOut, "CREATE TABLE IF NOT EXISTS staging(x TEXT)");
  run_sql(dbOut, "CREATE TABLE IF NOT EXISTS \"%w\"(x TEXT)", zOutTab);
  pIns = prepare_sql(dbOut, "INSERT INTO staging(x) VALUES(?1)");
  *pnRow = *pnStmt = 0;
  while( rc==SQLITE_OK && zSql && zSql[0] ){
    zLeftover = 0;
    rc = sqlite3_prepare_v2(dbRun, zSql, -1, &pStmt, &zLeftover);
    zSql = zLeftover;
    assert( rc==SQLITE_OK || pStmt==0 );
    if( rc!=SQLITE_OK ){
      printf("Error with [%s]\n%s\n", zSql, sqlite3_errmsg(dbRun));
      break;
    }
    if( !pStmt ) continue;
    (*pnStmt)++;
    nCol = sqlite3_column_count(pStmt);
    run_sql(dbOut, "DELETE FROM staging;");
    while( sqlite3_step(pStmt)==SQLITE_ROW ){
      int i, j;
      for(i=j=0; i<nCol && j<sizeof(zLine)-50; i++){
        int eType = sqlite3_column_type(pStmt, i);
        if( eType==SQLITE_NULL ){
          zCol = "NULL";
        }else{
          zCol = (const char*)sqlite3_column_text(pStmt, i);
        }
        if( i ) zLine[j++] = ',';
        if( eType==SQLITE_TEXT ){
          sqlite3_snprintf(sizeof(zLine)-j, zLine+j, "'%q'", zCol);
        }else{
          sqlite3_snprintf(sizeof(zLine)-j, zLine+j, "%s", zCol);
        }
        j += (int)strlen(zLine+j);
      }
      /* Detect if any row is too large and throw an error, because we will
      ** want to go back and look more closely at that case */
      if( j>=sizeof(zLine)-100 ){
        printf("Excessively long output line: %d bytes\n" ,j);
        exit(1);
      }
      if( bTrace ){
        printf("%s\n", zLine);
      }
      (*pnRow)++;
      sqlite3_bind_text(pIns, 1, zLine, j, SQLITE_TRANSIENT);
      rc = sqlite3_step(pIns);
      assert( rc==SQLITE_DONE );
      rc = sqlite3_reset(pIns);
    }
    run_sql(dbOut,
      "INSERT INTO \"%w\"(x) VALUES('### %q ###')",
      zOutTab, sqlite3_sql(pStmt)
    );
    run_sql(dbOut, 
      "INSERT INTO \"%w\"(x) SELECT group_concat(x,char(10))"
      "  FROM (SELECT x FROM staging ORDER BY x)",
      zOutTab
    );
    run_sql(dbOut, "COMMIT");
    sqlite3_finalize(pStmt);
    pStmt = 0;
  }
  sqlite3_finalize(pStmt);
  sqlite3_finalize(pIns);
  return rc;
}

/*
** Read the content of file zName into memory obtained from sqlite3_malloc64()
** and return a pointer to the buffer. The caller is responsible for freeing
** the memory.
**
** If parameter pnByte is not NULL, (*pnByte) is set to the number of bytes
** read.
**
** For convenience, a nul-terminator byte is always appended to the data read
** from the file before the buffer is returned. This byte is not included in
** the final value of (*pnByte), if applicable.
**
** NULL is returned if any error is encountered. The final value of *pnByte
** is undefined in this case.
*/
static char *readFile(const char *zName, int *pnByte){
  FILE *in = fopen(zName, "rb");
  long nIn;
  size_t nRead;
  char *pBuf;
  if( in==0 ) return 0;
  fseek(in, 0, SEEK_END);
  nIn = ftell(in);
  rewind(in);
  pBuf = sqlite3_malloc64( nIn+1 );
  if( pBuf==0 ) return 0;
  nRead = fread(pBuf, nIn, 1, in);
  fclose(in);
  if( nRead!=1 ){
    sqlite3_free(pBuf);
    return 0;
  }
  pBuf[nIn] = 0;
  if( pnByte ) *pnByte = nIn;
  return pBuf;
}

int main(int argc, char **argv){
  int nIn = 0;               /* Number of input files */
  char **azIn = 0;           /* Names of input files */
  sqlite3 *dbOut = 0;        /* Database to hold results */
  sqlite3 *dbRun = 0;        /* Database used for tests */
  int bTrace = 0;            /* Show query results */
  int nRow, nStmt;           /* Number of rows and statements */
  int i, rc;

  for(i=1; i<argc; i++){
    const char *z = argv[i];
    if( z[0]=='-' && z[1]=='-' ) z++;
    if( strcmp(z,"-help")==0 ){
      printf("Usage: %s [OPTIONS] FILENAME ...\n", argv[0]);
      printf("Options:\n");
      printf("  --help               Show his message\n");
      printf("  --output-trace       Show each line of SQL output\n");
      return 0;
    }
    else if( strcmp(z,"-output-trace")==0 ){
      bTrace = 1;
    }
    else if( z[0]=='-' ){
      printf("unknown option \"%s\".  Use --help for details\n", argv[i]);
      return 1;
    }
    else {
      nIn++;
      azIn = realloc(azIn, sizeof(azIn[0])*nIn);
      if( azIn==0 ){
        printf("out of memory\n");
        exit(1);
      }
      azIn[nIn-1] = argv[i];
    }
  }

  sqlite3_open(":memory:", &dbOut);
  sqlite3_open(":memory:", &dbRun);
  sqlite3_deserialize(dbRun, "main", data001, sizeof(data001),
                      sizeof(data001), SQLITE_DESERIALIZE_READONLY);
  for(i=0; i<nIn; i++){
    char *zSql = readFile(azIn[i], 0);
    sqlite3_stmt *pCk;
    sqlite3_test_control(SQLITE_TESTCTRL_OPTIMIZATIONS, dbRun, 0);
    if( bTrace ) printf("%s: Optimized\n", azIn[i]);
    rc = optfuzz_exec(dbRun, zSql, dbOut, "opt", &nStmt, &nRow, bTrace);
    if( rc ){
      printf("%s: optimized run failed: %s\n",
            azIn[i], sqlite3_errmsg(dbRun));
    }else{
      sqlite3_test_control(SQLITE_TESTCTRL_OPTIMIZATIONS, dbRun, 0xffff);
      if( bTrace ) printf("%s: Non-optimized\n", azIn[i]);
      rc = optfuzz_exec(dbRun, zSql, dbOut, "noopt", &nStmt, &nRow, bTrace);
      if( rc ){
        printf("%s: non-optimized run failed: %s\n",
              azIn[i], sqlite3_errmsg(dbRun));
        exit(1);
      }
      pCk = prepare_sql(dbOut,
           "SELECT (SELECT group_concat(x,char(10)) FROM opt)=="
           "       (SELECT group_concat(x,char(10)) FROM noopt)");
      rc = sqlite3_step(pCk);
      if( rc!=SQLITE_ROW ){
        printf("%s: comparison failed\n", sqlite3_errmsg(dbOut));
        exit(1);
      }
      if( !sqlite3_column_int(pCk, 0) ){
        printf("%s: opt/no-opt outputs differ\n", azIn[i]);
        pCk = prepare_sql(dbOut,
           "SELECT group_concat(x,char(10)) FROM opt "
           "UNION ALL "
           "SELECT group_concat(x,char(10)) FROM noopt");
        sqlite3_step(pCk);
        printf("opt:\n%s\n", sqlite3_column_text(pCk,0));
        sqlite3_step(pCk);
        printf("noopt:\n%s\n", sqlite3_column_text(pCk,0));
        exit(1);
      }else{
        printf("%s: %d stmts %d rows ok\n", azIn[i], nStmt, nRow);
      }
      sqlite3_finalize(pCk);
    }
    sqlite3_free(zSql);
  }
  sqlite3_close(dbRun);
  sqlite3_close(dbOut);    
  free(azIn);
  if( sqlite3_memory_used() ){
    printf("Memory leak of %lld bytes\n", sqlite3_memory_used());
    exit(1);
  }
  return 0;
}
