#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define IP_ADDR_LENGTH 16
#define QUERY_STRING_LENGTH 64
#define HOSTNAME_LENGTH 64
#define CHARSET "utf-8"
#define DB_FILE_NAME "test.db"

struct record_t
{
  char hostname[HOSTNAME_LENGTH];
  int ttl;
  char ip_addr[IP_ADDR_LENGTH]; 
};

// print HTTP header
static void
print_http_header( void )
{
  printf( "%s%s%c%c\n", "Content-Type. text/plain;charset=",CHARSET,13,10 );
}

// get environment variables safely
void
get_env_vars ( char *buf, const char *name, size_t size )
{
  debug("size: %d", size);
  char *value = getenv ( name );
  if ( value == NULL )
    log_err( "Can't get %s", name );
  else
    {
      debug( "%s:\t\t%s", name, value );
      strncpy(buf, value, size);
      buf[size-1] = '\0';
    }
}

// check if ip address is valid
static int
is_ip_address(char *ip_addr)
{
  char *octet = strtok ( ip_addr, "." );
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
  return 1;
}

// callback function for sql hostname query
static int
cb_hostname( void *client_hostname, int argc, char **argv, char**azColName )
{
  debug( "Arg[0]: %s", argv[0] );
  strncpy( (char*)client_hostname, argv[0], HOSTNAME_LENGTH - 1 );
  debug( "client_hostname: %s", (char*)client_hostname );

  return 0;
}

// get hostname for an id_string from sqlite3 database
static int
get_client_hostname ( char *client_hostname, const char *db_file_name,
                      const char *id_string )
{
  sqlite3 *db_conn;
  char *zErrMsg = 0;
  char sql[256];

  debug( "DB:\t\t%s", db_file_name );
  debug( "ID:\t\t%s", id_string );

  if( sqlite3_open( db_file_name, &db_conn ) != SQLITE_OK )
    {
      log_err( "Can't open database: %s", sqlite3_errmsg(db_conn));
      sqlite3_close( db_conn );
      return -1;
    }
  else
    {
      debug( "Successfully opened the database." );
    }

  snprintf( sql, sizeof(sql),
      "SELECT hostname FROM clients WHERE id_string='%s' LIMIT 1;", id_string );
  debug( "SQL statement:\t%s", sql );

  if( sqlite3_exec( db_conn, sql, cb_hostname, (void*)client_hostname,
        &zErrMsg ) != SQLITE_OK )
    {
      log_err( "SQL error:\t%s", zErrMsg);
      sqlite3_free(zErrMsg);
      sqlite3_close( db_conn );
      return -1;
    }

  sqlite3_close( db_conn );
  return 1;
}

int
main ( void ) {
  const char db_file_name[] = DB_FILE_NAME;
  char hostname[HOSTNAME_LENGTH] = "";
  char ip_addr[IP_ADDR_LENGTH] = ""; 
  char query_string[QUERY_STRING_LENGTH] = "";
  char buf[IP_ADDR_LENGTH] = "";

  get_env_vars(ip_addr, "REMOTE_ADDR", IP_ADDR_LENGTH);
  get_env_vars(query_string, "QUERY_STRING", QUERY_STRING_LENGTH);
  strncat(buf, ip_addr, sizeof(buf));

  if ( ! is_ip_address ( buf ) )
    {
      log_err( "Not a valid IP address: %s", ip_addr );
      return -1;
    }

  if( ! get_client_hostname( hostname, db_file_name, query_string ) )
    {
      log_err( "shit!" );
      return -1;
    }
  
  print_http_header();

  if ( hostname[0] == '\0' )
    printf( "Go away!\n" );  
  else
    printf( "Hostname:\t%s\n", hostname );
    printf( "IP:\t%s\n", ip_addr );
    printf( "ID:\t%s\n", query_string );

  // bye
  return 0;
}
