CC := /usr/bin/gcc
SQLITE := /usr/bin/sqlite3
PSQL := /usr/bin/psql
SHELL := /usr/bin/bash

DB_ENGINE := sqlite3
DB_FILE := ./dns.db
DB_FILE_TEST := ./test.db
DB_SCHEMA := ./db_schema.sql

REMOTE_ADDR := 192.168.234.234 
QUERY_STRING := qwertzuiop 

CFLAGS = -g -Ofast -std=gnu99 -Wall -Wextra -Wformat -Wno-format-extra-args \
				 -Wformat-security -Wformat-nonliteral -Wformat=2
LDFLAGS =
FLAGS = -DDB_ENGINE='"$(DB_ENGINE)"'
NDEBUG = -DNDEBUG
OBJ = ddns.o

ifeq ($(DB_ENGINE), sqlite3)
LDFLAGS += -lsqlite3
DB = $(DB_FILE)
DB_TEST = $(DB_FILE_TEST)
DB_CMD = $(SQLITE)
endif
ifeq ($(DB_ENGINE), psql)
LDFLAGS += -lpq
DB = dns
DB_TEST = testdb
DB_CMD = $(PSQL)
endif
ifndef DB_CMD
$(error No database command specified. Aborting!)
endif

all: proper ddns db

dev: proper testdb debug test

ddns: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(NDEBUG) $(FLAGS) -o $@.cgi $(OBJ)

debug: NDEBUG =
debug: FLAGS = -DDB_CONN_PARAMS='"$(DB_TEST)"'
debug: ddns

db:
	$(DB_CMD) $(DB) -init $(DB_SCHEMA) ""

testdb: DB = $(DB_TEST)
testdb: db
	$(DB_CMD) $(DB) \
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
test: TEST_ENV = REMOTE_ADDR=$(REMOTE_ADDR) QUERY_STRING=$(QUERY_STRING) 
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
