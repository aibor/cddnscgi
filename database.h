#ifndef __database_h__
#define __database_h__

int db_open ( const char *db_file_name, sqlite3 **db_conn );
int db_exec ( sqlite3 *db_conn, const char *sql_statement,
              sqlite3_callback, void *data );

#endif
