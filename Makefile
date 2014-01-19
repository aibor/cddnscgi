CC := /usr/bin/gcc
SQLITE := /usr/bin/sqlite3
SHELL := /usr/bin/bash

CFLAGS = -g -Ofast -std=gnu99 -Wall -Wextra -Wformat -Wno-format-extra-args -Wformat-security -Wformat-nonliteral -Wformat=2
LDFLAGS = -lsqlite3
NDEBUG = -DNDEBUG

OBJ = ddns.o

DB_FILE = dns.db
DB_FILE_TEST = test.db

DB_SCHEMA = ./db_schema.sql

REMOTE_ADDR := 192.168.234.234 
QUERY_STRING := qwertzuiop 
TEST_ENV = REMOTE_ADDR=$(REMOTE_ADDR) QUERY_STRING=$(QUERY_STRING)

all: proper ddns db

dev: proper testdb debug test

ddns: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(NDEBUG) $(FLAGS) -o $@.cgi $(OBJ)

debug: NDEBUG =
debug: FLAGS = -DDB_FILE_NAME='"$(DB_FILE_TEST)"'
debug: ddns

db:
	$(SQLITE) $(DB_FILE) -init $(DB_SCHEMA) ""

testdb: DB_FILE = $(DB_FILE_TEST)
testdb: db
	$(SQLITE) $(DB_FILE) \
		"INSERT INTO domains(domain_name) \
			VALUES ('example.com.'); \
		 INSERT INTO records_j(name, domain, type, content) \
		 	VALUES ('test', 'example.com.', 'A', '10.0.0.1'); \
		 INSERT INTO ddns_clients(id_string, record_id) \
			VALUES ('qwertzuiop', ( \
				SELECT id \
				FROM records_j \
				WHERE name = 'test' \
				AND domain = 'example.com.' \
				AND type = 'A' \
			));"

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(NDEBUG) $(FLAGS) -c $<

.PHONY: test valgrind clean proper
test:
	@start=$$(date '+%s.%N'); \
	time $(TEST_ENV) ./ddns.cgi; \
	stop=$$(date '+%s.%N'); \
	echo -e "\033[0;34mLaufzeit:\033[0m "$$(bc <<< "$$stop - $$start")" seconds"

valgrind:
	$(TEST_ENV) valgrind ./ddns.cgi

clean:
	-rm -f ddns.cgi *.o

proper: clean
	-rm -f $(DB_FILE) $(DB_FILE_TEST)
