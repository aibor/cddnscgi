#ifndef __ddns_h__
#define __ddns_h__
#define IP_ADDR_LENGTH 16
#define QUERY_STRING_LENGTH 64
#define HOSTNAME_LENGTH 64
#define CHARSET "utf-8"
#ifdef NDEBUG
#define DB_FILE_NAME ".clients.db"
#else
#define DB_FILE_NAME "test.db"
#endif

struct host
{
  char name[HOSTNAME_LENGTH];
  char ip_addr[IP_ADDR_LENGTH]; 
};


#endif
