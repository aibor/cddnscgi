#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "debug.h"

#define QUOTE(...) #__VA_ARGS__
#define UNUSED(x) (void)(x)

#ifndef IP_ADDR_LENGTH
#define IP_ADDR_LENGTH 16
#endif
#ifndef QUERY_STRING_LENGTH
#define QUERY_STRING_LENGTH 64
#endif
#ifndef HOSTNAME_LENGTH
#define HOSTNAME_LENGTH 64
#endif
#ifndef SQL_STATEMENT_LENGTH
#define SQL_STATEMENT_LENGTH 256
#endif
#ifndef CHARSET
#define CHARSET "utf-8"
#endif

#if DB_ENGINE == sqlite3
#ifndef DB_CONN_PARAMS
#define DB_CONN_PARAMS "dns.db"
#endif
#define DB_OPEN(C,P) { sqlite3_open(P, C); }
#define DB_OPEN_OK SQLITE_OK
#define DB_CLOSE(C) { sqlite3_close(C); }
#define DB_TYPE sqlite3
#elif DB_ENGINE == psql
#ifndef DB_CONN_PARAMS
#define DB_CONN_PARAMS "dbname = dns"
#endif
#define DB_OPEN
#define DB_CLOSE(C) { PQfinish(C); }
#define DB_TYPE PGconn 
#endif

#ifndef IP_UPDATE_STMT
#define IP_UPDATE_STMT "UPDATE records SET content = '%s' WHERE record_id = %s;",\
                       client->ip_addr, client->record_id
#endif
#ifndef CLIENT_QUERY_STMT
#define CLIENT_QUERY_STMT "SELECT r.record_id, r.record_name, r.content "\
                          "FROM ddns_clients dd "\
                          "INNER JOIN records r USING(record_id) "\
                          "WHERE dd.id_string = '%s';", id_string
#endif

/*
 * struct which holds all information for a respective client
 */
struct client
{
  char record_id[8],
       hostname[HOSTNAME_LENGTH],
       ip_addr[IP_ADDR_LENGTH]; 
};

/*
 * wrapper function for opening a sqlite3 database
 */
static int
db_open(const char *db_conn_params, DB_TYPE **db_conn)
{
  debug("Open DB with params:\t\t%s", db_conn_params);

  if ((DB_OPEN(db_conn, db_conn_params)) != DB_OPEN_OK) 
  {
    log_err("Can't open database: %s", sqlite3_errmsg(*db_conn));
    DB_CLOSE(*db_conn);
    return 1;
  } 
  else
  {
    debug("\033[0;32mSuccessfully opened the database.\033[0m");
    return 0;
  }
}

/*
 * wrapper function for exectuting any SQL statement
 */
static int
db_exec(DB_TYPE *db_conn, const char *sql_statement,
    sqlite3_callback callback, void *data)
{
  char *zErrMsg = 0;

  debug("Executing SQL statement:\n\t\033[0;33m%s\033[0m", sql_statement);

  if(sqlite3_exec(db_conn, sql_statement, callback, data, &zErrMsg)
      != SQLITE_OK)
  {
    log_err("SQL error:\t%s", zErrMsg);
    sqlite3_free(zErrMsg);
    DB_CLOSE(db_conn);
    return 1;
  }
  else
  {
    debug("\033[0;32mSuccessfully executed the SQL statement.\033[0m");
    return 0;
  }
}

/*
 * set database pragmas for performance increase
 */
static int
db_optimize(DB_TYPE *db_conn)
{
  char *sql = QUOTE(
      PRAGMA foreign_keys=ON;
      PRAGMA locking_mode=EXCLUSIVE;
      PRAGMA synchronous=OFF;
      );

  return db_exec(db_conn, sql, 0, 0);
}

/*
 * print the HTTP header to stdout
 */
static void
print_http_header(void)
{
  printf("%s%s%c%c\n", "Content-Type. text/plain;charset=",CHARSET,13,10);
}

/*
 * get environment variables safely
 */
static int
get_env_vars(char *buf, const char *name, size_t size)
{
  char *value = getenv (name);
  unsigned int i = 0;

  if (value == NULL)
    return 1;

  if (strncpy(buf, value, size) == NULL)
    return 2;

  for ( ; i < size; i++)
    if ((buf[i] =='\'') || (i == (size -1)))
      buf[i] = '\0';

  return 0;
}

/*
 * function for checking if the passed string is a valid ip address
 */
static int
validate_ip_address(const char *ip_addr)
{
  char buf[IP_ADDR_LENGTH],
       *octet;
  int  i = 0;

  if (strncat(buf, ip_addr, sizeof(buf) - 1) == NULL)
    return 1;

  octet = strtok (buf, ".");

  while (octet != NULL)
  {
    if(atoi(octet) < 0x0 || atoi(octet) > 0xff || i > 3)
      return 2;

    octet = strtok(NULL, ".");
    i++;
  }

  return (i == 4) ? 0 : 3;
}

/*
 * callback function for SQL client query
 */
static int
cb_hostname(void *client_v, int argc, char **argv, char **azColName)
{
  struct client *client = (struct client *)client_v;
  //UNUSED(argc);
  UNUSED(azColName);

  debug("argc: %d", argc);

  strncpy(client->record_id, argv[0] ? argv[0] : "", 7);
  strncpy(client->hostname, argv[1] ? argv[1] : "", HOSTNAME_LENGTH - 1);
  strncpy(client->ip_addr, argv[2] ? argv[2] : "", IP_ADDR_LENGTH - 1);

  return 0;
}

/*
 * query client information from sqlite3 database
 */
static int
db_get_client(DB_TYPE *db_conn, struct client *client, const char *id_string)
{
  char sql[SQL_STATEMENT_LENGTH];

  sprintf(sql, CLIENT_QUERY_STMT);

  return db_exec(db_conn, sql, cb_hostname, client);
}

/*
 * update the Ip address for a client
 */
static int
db_set_ip(DB_TYPE *db_conn, struct client *client)
{
  char sql[SQL_STATEMENT_LENGTH];

  sprintf(sql, IP_UPDATE_STMT);

  return db_exec(db_conn, sql, 0, 0);
}

int
main()
{
  debug("\033[1;32mStarting...\033[0m");

  const char *db_conn_params = DB_CONN_PARAMS;
  struct      client client;
  char        ip_addr[IP_ADDR_LENGTH], 
              query_string[QUERY_STRING_LENGTH];
  DB_TYPE    *db_conn = 0;

  if (get_env_vars(ip_addr, "REMOTE_ADDR", IP_ADDR_LENGTH) != 0)
    die(1, "Can't get remote address");

  if (get_env_vars(query_string, "QUERY_STRING", QUERY_STRING_LENGTH) != 0)
    die(2, "Can't get query string");

  if (validate_ip_address(ip_addr) != 0)
    die(3, "Not a valid IP address: %s", ip_addr);

  if (db_open(db_conn_params, &db_conn) != 0)
    die(4, "No database, no work!");

  if (db_optimize(db_conn) != 0)
    log_err("Failed to set pragmas for increased performance!");

  if (db_get_client(db_conn, &client, query_string) != 0)
  {
    DB_CLOSE(db_conn);
    die(5, "shit!");
  }

  print_http_header();

  debug("Hostname:\t\t%s", client.hostname);
  debug("IP:\t\t%s", client.ip_addr);
  debug("ID:\t\t%s", query_string);

  if (client.hostname[0] == '\0')
    printf("Go away!\n");  
  else if (strcmp(ip_addr, client.ip_addr))
  {
    if ((strcpy(client.ip_addr, ip_addr) == NULL) ||
        (db_set_ip(db_conn, &client) != 0))
      printf("Failed to set new IP address.\n");
    else
      printf("Successfully updated the IP address.\n");
  } 
  else
    printf("IP address already known.\n");

  // bye
  DB_CLOSE(db_conn);
  debug("\033[1;32mFinishing...\033[0m");
  return 0;
}
