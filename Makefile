CC := /usr/bin/gcc
SQLITE := /usr/bin/sqlite3
PSQL := /usr/bin/psql
SHELL := /usr/bin/bash

DB_ENGINE := SQLITE3
#DB_ENGINE := PSQL
DB_FILE := ./dns.db
DB_FILE_TEST := ./test.db

REMOTE_ADDR := 192.168.234.234 
QUERY_STRING := qwertzuiop 

TEST_ENV = REMOTE_ADDR=$(REMOTE_ADDR) QUERY_STRING=$(QUERY_STRING) 

CFLAGS = -g -Ofast -std=gnu99 -Wall -Wextra -Wformat -Wno-format-extra-args \
				 -Wformat-security -Wformat-nonliteral -Wformat=2 -Wunused-macros -Wundef \
				 -Wendif-labels -Werror -pedantic

LDFLAGS =
FLAGS = -D$(DB_ENGINE)
NDEBUG = -DNDEBUG
OBJ = ddns.o

ifndef DB_ENGINE
	$(error No database engine spcified. Aborting!)
endif
ifeq ($(DB_ENGINE), SQLITE3)
	LDFLAGS += -lsqlite3
	DB = $(DB_FILE)
	DB_TEST = $(DB_FILE_TEST)
	DB_CMD = $(SQLITE)
	DB_SCHEMA := ./db_sqlite3.sql
	DB_CMD_PARAMS = -init $(DB_SCHEMA) ""
else  
ifeq ($(DB_ENGINE), PSQL)
	LDFLAGS += -lpq
	DB = dns
	DB_TEST = testdb
	DB_CMD = $(PSQL)
	DB_SCHEMA := ./db_psql.sql
	DB_CMD_PARAMS = -f $(DB_SCHEMA)
endif
endif
ifndef DB_CMD
	$(error No database command specified. Aborting!)
endif

all: proper ddns db

dev: DB = $(DB_TEST)
dev: proper debug testdb test

ddns: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(NDEBUG) $(FLAGS) -o $@.cgi $(OBJ)

debug: NDEBUG =
ifeq ($(DB_ENGINE), SQLITE3)
debug: FLAGS += -DDB_CONN_PARAMS='"$(DB_TEST)"'
else
ifeq ($(DB_ENGINE), PSQL)
debug: FLAGS += -DDB_CONN_PARAMS='"dbname=$(DB_TEST)"'
endif
endif
debug: ddns

db:
	$(DB_CMD) $(DB) $(DB_CMD_PARAMS)

testdb: DB = $(DB_TEST)
testdb: db
	$(DB_CMD) $(DB) <<< \
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
	$(TEST_ENV) valgrind -v ./ddns.cgi

clean:
	-rm -f ddns.cgi *.o

ifeq ($(DB_ENGINE), SQLITE3)
proper: clean
	-rm -f $(DB_FILE) $(DB_FILE_TEST)
else
ifeq ($(DB_ENGINE), PSQL)
proper: clean
	$(DB_CMD) $(DB) <<< \
		"DROP TABLE domains CASCADE; \
		 DROP TABLE records CASCADE; \
		 DROP TABLE types CASCADE; \
		 DROP TABLE ddns_clients CASCADE; \
		 DROP TABLE content_logs CASCADE; \
		 DROP FUNCTION increment_serial(); \
		 DROP FUNCTION insert_records_j(); \
		 DROP FUNCTION log_content_change(); \
		 DROP FUNCTION sanitize_domain_input(); \
		 DROP FUNCTION update_date(); \
		 DROP FUNCTION update_record();"
endif
endif
