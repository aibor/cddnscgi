CFLAGS=-Wall -g -Wextra
LDFLAGS=-lsqlite3

ddns: ddns.c
	cc $(CFLAGS) $(LDFLAGS) -o ddns.cgi ddns.c

ndebug: CFLAGS+=-DNDEBUG
ndebug: ddns

.PHONY: test
test:
	bash -c 'time REMOTE_ADDR=192.168.234.12 QUERY_STRING=qwertzuiop ./ddns.cgi'

valgrind:
	REMOTE_ADDR=192.168.234.12 QUERY_STRING=qwertzuiop valgrind ./ddns.cgi

.PHONY: clean
clean:
	rm ddns.cgi
