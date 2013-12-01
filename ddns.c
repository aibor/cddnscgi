#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "debug.h"
//#include "ddns.h"
//#include "database.h"

#define IP_ADDR_LENGTH 16
#define QUERY_STRING_LENGTH 64
#define HOSTNAME_LENGTH 64
#define CHARSET "utf-8"
#ifdef NDEBUG
#define DB_FILE_NAME ".clients.db"
#else
#define DB_FILE_NAME "test.db"
#endif

/*
 * struct which holds all information for a respective client
 */
struct client
{
  char hostname[HOSTNAME_LENGTH];
  char ip_addr[IP_ADDR_LENGTH]; 
};

/*
 * wrapper function for opening a sqlite3 database
 */
static int
db_open ( const char *db_file_name, sqlite3 **db_conn )
{
  debug( "Open DB:\t\t%s", db_file_name );

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

/*
 * wrapper function for exectuting any SQL statement
 */
static int
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

/*
 * print the HTTP header to stdout
 */
static void
print_http_header( void )
{
  printf( "%s%s%c%c\n", "Content-Type. text/plain;charset=",CHARSET,13,10 );
}

/*
 * get environment variables safely
 */
static void
get_env_vars ( char *buf, const char *name, size_t size )
{
  char *value = getenv ( name );
  if ( value == NULL )
    log_err( "Can't get %s", name );
  else
    {
      debug( "%s:\t%s", name, value );
      strncpy(buf, value, size);
      buf[size-1] = '\0';
    }
}

/*
 * function for checking if the passed string is a valid ip address
 */
static int
is_ip_address(const char *ip_addr)
{
  char buf[IP_ADDR_LENGTH] = "";
  strncat( buf, ip_addr, sizeof(buf)-1 );
  char *octet = strtok ( buf, "." );
  int i = 0;

  while ( octet != NULL )
    {
      if( atoi(octet) < 0x0
          || atoi(octet) > 0xff
          || i > 3 )
        return 0;
      octet = strtok(NULL, ".");
      i++;
    }

  return ( i != 4 ) ? 0 : 1;
}

/*
 * callback function for SQL client query
 */
static int
cb_hostname( void *client_v, int argc, char **argv, char **azColName )
{
  struct client *client = (struct client *)client_v;
  strncpy( client->hostname, argv[0] ? argv[0] : "", HOSTNAME_LENGTH - 1 );
  strncpy( client->ip_addr, argv[1] ? argv[1] : "", IP_ADDR_LENGTH - 1 );
  debug( "client->hostname:\t%s", client->hostname );
  debug( "client->ip_addr:\t%s", client->ip_addr );

  return 0;
}

/*
 * query client information from sqlite3 database
 */
static int
db_get_client ( sqlite3 *db_conn, struct client *client, const char *id_string )
{
  char sql[256] = "";
  sprintf( sql,
           "SELECT hostname,ip FROM clients WHERE id_string='%s' LIMIT 1;",
           id_string );

  return ( db_exec( db_conn, sql, cb_hostname, client ) ) ? 1 : 0;
}

/*
 * update the Ip address for a client
 */
static int
db_set_ip ( sqlite3 *db_conn, struct client *client )
{
  char sql[256] = "";
  sprintf( sql,
           "UPDATE clients set ip = '%s' WHERE hostname = '%s';",
           client->ip_addr, client->hostname );
  return ( db_exec( db_conn, sql, 0, 0 ) ) ? 1 : 0;
}

int
main () {
  debug("\033[1;32mStarting...\033[0m");
  const char db_file_name[] = DB_FILE_NAME;
  struct client client = {"",""};
  char ip_addr[IP_ADDR_LENGTH] = ""; 
  char query_string[QUERY_STRING_LENGTH] = "";
  sqlite3 *db_conn = 0;

  get_env_vars(ip_addr, "REMOTE_ADDR", IP_ADDR_LENGTH);
  get_env_vars(query_string, "QUERY_STRING", QUERY_STRING_LENGTH);

  if ( ! is_ip_address( ip_addr ) )
    {
      log_err( "Not a valid IP address: %s", ip_addr );
      return 1;
    }

  if( db_open( db_file_name, &db_conn ) )
    return 2;

  if( db_get_client( db_conn, &client, query_string ) )
    {
      log_err( "shit!" );
      sqlite3_close( db_conn );
      return 4;
    }
  
  print_http_header();

  debug( "Hostname:\t\t%s", client.hostname );
  debug( "IP:\t\t%s", client.ip_addr );
  debug( "ID:\t\t%s", query_string );

  if ( client.hostname[0] == '\0' )
    printf( "Go away!\n" );  
  else if ( strcmp(ip_addr, client.ip_addr) )
    {
      strcpy(client.ip_addr, ip_addr);
      printf( ( db_set_ip( db_conn, &client ) )
          ? "Failed to set new IP address.\n"
          : "Successfully updated the IP address.\n" );
    }
  else
    printf( "IP address already known.\n" );

  // bye
  sqlite3_close( db_conn );
  debug("\033[1;32mFinishing...\033[0m");
  return 0;
}
