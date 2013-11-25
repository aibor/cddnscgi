CC = /usr/bin/gcc
SQLITE = /usr/bin/sqlite3

CFLAGS = -Wall -g -Wextra
LDFLAGS = -lsqlite3
NDEBUG = -DNDEBUG

OBJ = $(shell ls *.c | sed 's/.c/.o/')

DB_FILE = ./.clients.db
DB_FILE_TEST = ./test.db


ddns: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(NDEBUG) -o $@.cgi $(OBJ)

debug: NDEBUG =
debug: ddns

db:
	$(SQLITE) $(DB_FILE) "create table clients ( \
		id integer primary key autoincrement, \
		hostname varchar(64), \
		id_string varchar(64), \
		ip varchar(15) );"

testdb: DB_FILE = $(DB_FILE_TEST)
testdb: db
	$(SQLITE) $(DB_FILE) \
		"insert into clients ( hostname, id_string, ip )	\
		values ( 'home.example.com', 'qwertzuiop', '10.0.0.1' );"

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(NDEBUG) -c $<

.PHONY: test valgrind clean proper
test:
	bash -c 'time REMOTE_ADDR=192.168.234.12 QUERY_STRING=qwertzuiop ./ddns.cgi'

valgrind:
	REMOTE_ADDR=192.168.234.12 QUERY_STRING=qwertzuiop valgrind ./ddns.cgi

clean:
	-rm -f ddns.{cgi,o}

proper: clean
	-rm -f $(DB_FILE) $(DB_FILE_TEST)
