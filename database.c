#include <stdio.h>
#include <sqlite3.h>
#include "ddns.h"
#include "debug.h"

int
db_open ( const char *db_file_name, sqlite3 **db_conn )
{
  debug( "Open DB:\t%s", db_file_name );

  if( sqlite3_open( db_file_name, db_conn ) != SQLITE_OK )
    {
      log_err( "Can't open database: %s", sqlite3_errmsg(*db_conn));
      sqlite3_close( *db_conn );
      return 1;
    }
  else
    {
      debug( "\033[0;32mSuccessfully opened the database.\033[0m" );
      return 0;
    }
}

int
db_exec ( sqlite3 *db_conn, const char *sql_statement,
          sqlite3_callback callback, void *data )
{
  char *zErrMsg = 0;

  debug( "Executing SQL statement:\n\t\033[0;33m%s\033[0m", sql_statement );

  if( sqlite3_exec( db_conn, sql_statement, callback, data, &zErrMsg )
      != SQLITE_OK )
    {
      log_err( "SQL error:\t%s", zErrMsg );
      sqlite3_free(zErrMsg);
      sqlite3_close( db_conn );
      return 1;
    }
  else
    {
      debug( "\033[0;32mSuccessfully executed the SQL statement.\033[0m" );
      return 0;
    }

}
