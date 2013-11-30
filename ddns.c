#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "debug.h"
#include "ddns.h"
#include "database.h"

// print HTTP header
static void
print_http_header( void )
{
  printf( "%s%s%c%c\n", "Content-Type. text/plain;charset=",CHARSET,13,10 );
}

// get environment variables safely
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

  if ( i != 4 )
    return 0;
  else
    return 1;
}

// callback function for sql hostname query
int
cb_hostname( void *host_v, int argc, char **argv, char **azColName )
{
  struct host *host = (struct host *)host_v;
  strncpy( host->name, argv[0] ? argv[0] : "", HOSTNAME_LENGTH - 1 );
  strncpy( host->ip_addr, argv[1] ? argv[1] : "", IP_ADDR_LENGTH - 1 );
  debug( "client->hostname:\t%s", host->name );
  debug( "client->ip_addr:\t%s", host->ip_addr );

  return 0;
}

// get hostname for an id_string from sqlite3 database
static int
db_get_client ( struct host *host, sqlite3 *db_conn, const char *id_string )
{
  char sql[256] = "";

  sprintf( sql, "SELECT hostname,ip FROM clients WHERE id_string='%s' LIMIT 1;",
           id_string );

  if( db_exec( db_conn, sql, cb_hostname, host ) )
    return 1;

  return 0;
}

static int
db_set_ip ( struct host *host, sqlite3 *db_conn )
{
  char sql[256] = "";

  sprintf( sql, "UPDATE clients set ip = '%s' WHERE hostname = '%s';",
           host->ip_addr, host->name );

  if( db_exec( db_conn, sql, 0, 0 ) )
    return 1;

  return 0;
}

int
main ( void ) {
  const char db_file_name[] = DB_FILE_NAME;
  struct host client = {"",""};
  char ip_addr[IP_ADDR_LENGTH] = ""; 
  char query_string[QUERY_STRING_LENGTH] = "";
  char buf[IP_ADDR_LENGTH] = "";
  sqlite3 *db_conn = 0;

  get_env_vars(ip_addr, "REMOTE_ADDR", IP_ADDR_LENGTH);
  get_env_vars(query_string, "QUERY_STRING", QUERY_STRING_LENGTH);
  strncat(buf, ip_addr, sizeof(buf));

  if ( ! is_ip_address( buf ) )
    {
      log_err( "Not a valid IP address: %s", ip_addr );
      return 1;
    }

  if( db_open( db_file_name, &db_conn ) )
    return 2;

  if( db_get_client( &client, db_conn, query_string ) )
    {
      log_err( "shit!" );
      sqlite3_close( db_conn );
      return 4;
    }
  
  print_http_header();

  if ( client.name[0] == '\0' )
    printf( "Go away!\n" );  
  else if ( strcmp(ip_addr, client.ip_addr) )
    {
      strcpy(client.ip_addr, ip_addr);
      if ( db_set_ip( &client, db_conn ) )
        printf( "Failed to set new IP address.\n" );
      else
        printf( "Successfully updated the IP address.\n" );
    }
  else
    printf( "IP address already known." );

  debug( "Hostname:\t%s", client.name );
  debug( "IP:\t\t%s", client.ip_addr );
  debug( "ID:\t\t%s", query_string );

  // bye
  sqlite3_close( db_conn );
  return 0;
}
